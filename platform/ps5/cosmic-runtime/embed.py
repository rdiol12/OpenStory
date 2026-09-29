#!/usr/bin/env python3
"""Link the experimental Java hardware probe as a native application (not Cosmic)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from static_symbols import generate

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, default=Path.home() / 'build/cosmic-ps5')
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--ps5library', type=Path, default=HERE.parents[3] / 'PS5Library')
    args = parser.parse_args()
    work, sdk, library = (p.resolve() for p in (args.work, args.sdk, args.ps5library))
    build = work / 'jdk-static-build'
    native = library / 'upstream/ProsperoTV/tooling/native'
    output = work / 'native-probe'
    output.mkdir(exist_ok=True)
    env = dict(os.environ, PS5_PAYLOAD_SDK=str(sdk), LLVM_CONFIG='/usr/bin/llvm-config-18')

    def run(command, log):
        print(f'{log}: {command[0]}', flush=True)
        with (output / log).open('w') as stream:
            result = subprocess.run(list(map(str, command)), env=env, cwd=output,
                                    stdout=stream, stderr=subprocess.STDOUT)
        if result.returncode:
            print('\n'.join((output / log).read_text(errors='replace').splitlines()[-35:]))
            raise SystemExit(f'Failed; see {output / log}')

    libdir = build / 'support/modules_libs/java.base'
    archives = [libdir / f'lib{name}.a' for name in ('java', 'net', 'nio', 'zip', 'jimage', 'verify')]
    archives.append(libdir / 'zero/libjvm.a')
    count = generate(archives, output / 'static_symbols.inc')
    layout = (native / 'ps5-pie.ld').read_text()
    for section in ('eh_frame', 'eh_frame_hdr'):
        marker = f'KEEP(*(.{section}))'
        if layout.count(marker) != 1:
            raise SystemExit('Unexpected native linker layout')
        layout = layout.replace(marker, f'__{section}_start = .; {marker} __{section}_end = .;')
    (output / 'native.ld').write_text(layout)
    includes = [f'-I{output}', f'-I{build}/support/modules_include/java.base',
                f'-I{build}/support/modules_include/java.base/freebsd']
    objects = []
    sources = [HERE / 'probe.c', HERE / 'static_runtime.c', HERE.parent / 'Allocation.c',
               native / 'app_crt.cpp', library / 'ps5/build/native-app/app_cpp_runtime.cpp']
    for source in sources:
        obj = output / (source.stem + '.o')
        compiler = work / 'bin' / ('ps5-cxx' if source.suffix == '.cpp' else 'ps5-cc')
        run([compiler, '-O2', '-fPIC', '-ffunction-sections', '-fdata-sections',
             '-fno-builtin', *includes, '-c', source, '-o', obj], f'compile-{source.stem}.log')
        objects.append(obj)
    intermediate = output / 'java-probe.elf'
    run([work / 'bin/ps5-cxx', '-pie', '-nostdlib', '-nostartfiles',
         '-Wl,-z,defs,--gc-sections,--exclude-libs=ALL,-e,_start',
         f'-Wl,-T,{output}/native.ld', f'-Wl,--version-script,{native}/app-symbols.map',
         '-Wl,--defsym=__cosmic_image_start=ADDR(.text)',
         '-Wl,--defsym=__cosmic_image_end=ADDR(.bss)+SIZEOF(.bss)',
         *objects, '-Wl,--start-group', *archives, work / 'ffi-install/lib/libffi.a',
         '-lkernel', '-lSceNet', '-lc++', '-lc++abi', '-lunwind', '-lc',
         '-Wl,--end-group', '-lSceLibcInternal', '-lkernel', '-o', intermediate], 'link.log')
    symbols = subprocess.check_output(['llvm-nm-18', str(intermediate)], text=True)
    for forbidden in ('kernel_mprotect', 'kernel_get_fw_version', '__dlopen', '__dlsym', '__dladdr', '__dlclose', '__dlerror'):
        if any(line.split()[-1:] == [forbidden] for line in symbols.splitlines()):
            raise SystemExit(f'Unexpected payload dependency or unresolved loader hook: {forbidden}')
    run(['llvm-readelf-18', '-h', '-l', '-d', intermediate], 'elf.log')
    run(['llvm-nm-18', '--undefined-only', intermediate], 'imports.log')
    tool = output / 'ps5-native-tool'
    run(['clang++-18', '-std=c++20', '-O2',
         *[native / name for name in ('native_app_builder.cpp', 'self_container.cpp', 'elf_object.cpp', 'sce_module_writer.cpp')],
         '-lz', '-o', tool], 'build-converter.log')
    run([tool, 'link', '--in', intermediate, '--out', output / 'eboot.elf',
         '--stub-dir', sdk / 'target/lib', '--module-sdk', '0x02000009',
         '--companion-sdk', '0x08050001', '--file-name', 'eboot.elf'], 'convert.log')
    run([tool, 'self', '--sign', '--in', output / 'eboot.elf', '--out', output / 'eboot.bin',
         '--magic', '0x1D3D154F'], 'container.log')
    run([tool, 'self', '--extract', '--file', output / 'eboot.bin', '--out', output / 'readback.elf'], 'readback.log')
    sys.path.insert(0, str(library / 'ps5/native'))
    from verify import verify_readback
    verify_readback((output / 'eboot.elf').read_bytes(), (output / 'readback.elf').read_bytes())
    classes = output / 'probe'
    classes.mkdir(exist_ok=True)
    run(['javac', '--release', '21', '-d', classes, HERE / 'RuntimeProbe.java'], 'compile-java.log')
    with tempfile.TemporaryDirectory() as temporary:
        for iteration in range(2):
            run(['java', f'-Djava.io.tmpdir={temporary}', '-cp', classes, 'RuntimeProbe'], f'host-probe-{iteration}.log')
    review = HERE.parents[2] / 'build/ps5/cosmic-runtime'
    app = review / 'dist/PPSA99784'
    (app / 'sce_sys').mkdir(parents=True, exist_ok=True)
    (app / 'sce_module').mkdir(exist_ok=True)
    shutil.copyfile(output / 'eboot.bin', app / 'eboot.bin')
    shutil.copyfile(library / 'ps5/build/native-app/dist/PPSA99051/sce_module/libc.prx', app / 'sce_module/libc.prx')
    shutil.copytree(classes, app / 'probe', dirs_exist_ok=True)
    shutil.copytree(build / 'jdk/modules/java.base', app / 'java/modules/java.base', dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('_the.*'))
    shutil.copytree(build / 'support/modules_conf/java.base', app / 'java/conf', dirs_exist_ok=True)
    (app / 'java/lib').mkdir(exist_ok=True)
    shutil.copyfile(libdir / 'tzdb.dat', app / 'java/lib/tzdb.dat')
    shutil.copytree(libdir / 'security', app / 'java/lib/security', dirs_exist_ok=True)
    source = work / 'openjdk21-bsd'
    for name in ('LICENSE', 'ASSEMBLY_EXCEPTION'):
        shutil.copyfile(source / name, app / 'java' / name)
    shutil.copytree(source / 'src/java.base/share/legal', app / 'java/legal/java.base', dirs_exist_ok=True)
    shutil.copyfile(work / 'libffi/LICENSE', app / 'java/legal/libffi-LICENSE')
    # Use the established homebrew application's metadata layout with a separate title ID.
    param = json.loads((HERE.parents[2] / 'build/ps5/dist/PPSA99783/sce_sys/param.json').read_text())
    param.update(titleId='PPSA99784', conceptId='99784', contentId='UP9000-PPSA99784_00-COSMICJAVAPROBE0')
    param['localizedParameters'] = {'defaultLanguage': 'en-US', 'en-US': {'titleName': 'Cosmic Java Probe'}}
    (app / 'sce_sys/param.json').write_text(json.dumps(param, indent=2) + '\n')
    shutil.copyfile(HERE.parent / 'icon0.png', app / 'sce_sys/icon0.png')
    for name in ('eboot.elf', 'elf.log', 'imports.log', 'convert.log', 'container.log',
                 'readback.log', 'host-probe-0.log', 'host-probe-1.log'):
        shutil.copyfile(output / name, review / name)
    status = {
        'kind': 'java-runtime-probe', 'cosmicServer': False, 'hardwareTested': False,
        'installablePkg': False, 'targetFirmware': '4.50', 'nativeReferences': count,
        'ebootSha256': hashlib.sha256((output / 'eboot.elf').read_bytes()).hexdigest(),
        'containerReadbackVerified': True,
        'classCount': len(list((app / 'java/modules/java.base').rglob('*.class'))),
        'containerSha256': hashlib.sha256((app / 'eboot.bin').read_bytes()).hexdigest(),
        'packagingBlocker': 'No title-specific nptitle.dat; no PKG has been produced for this probe.',
        'remaining': ['Native runtime execution on PS5', 'Full Java modules, GraalJS and database',
                      'Cosmic startup and client login', 'Complete application packaging'],
    }
    (output / 'status.json').write_text(json.dumps(status, indent=2) + '\n')
    (review / 'status.json').write_text(json.dumps(status, indent=2) + '\n')
    shutil.copyfile(HERE / 'README.md', review / 'README.md')
    print(f'Native probe linked: {output}/eboot.elf (console untested; not a server or ELF-loader payload).')
    print(f'Application folder staged: {app}; no installable PKG created.')


if __name__ == '__main__':
    main()
