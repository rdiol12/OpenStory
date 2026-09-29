#!/usr/bin/env python3
"""Experimental PS5 Java cross-build in Linux/WSL; does not package Cosmic."""
import argparse
import hashlib
import os
from pathlib import Path
import shlex
import subprocess

HERE = Path(__file__).resolve().parent
SOURCES = {
    'openjdk21-bsd': (
        'https://codeload.github.com/battleblow/jdk21u/tar.gz/refs/tags/jdk-21.0.12%2B8-2',
        'ac74fa27c3e368d5a3af4f537f8b68a830907a8da1541d167dccf27aa7ea7037'),
    'libffi': (
        'https://github.com/libffi/libffi/releases/download/v3.8.0/libffi-3.8.0.tar.gz',
        '7da3e2d9a171eb0a038f592ecad3ff2bb2550f3496d87b3b29ad0cf4430c0db4'),
}


def patch_is_applied(source, patch):
    return subprocess.run(['patch', '--force', '--fuzz=0', '--dry-run', '-R', '-p1', '-i', str(patch)],
                          cwd=source, capture_output=True).returncode == 0


def write_script(path, text):
    if not path.is_file() or path.read_text() != text:
        path.write_text(text)
    path.chmod(0o755)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, default=Path.home() / 'build/cosmic-ps5')
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--jdk', type=Path, default=Path('/usr/lib/jvm/java-21-openjdk-amd64'))
    parser.add_argument('--stage', choices=('all', 'ffi', 'hotspot', 'base'), default='all')
    parser.add_argument('--jobs', type=int, default=8)
    parser.add_argument('--static', action='store_true', help='Build archives for an embedded native application.')
    args = parser.parse_args()
    if os.name != 'posix' or args.jobs < 1:
        parser.error('Use Linux/WSL with a positive job count.')
    work, sdk, jdk = (p.resolve() for p in (args.work, args.sdk, args.jdk))
    if not (sdk / 'bin/prospero-clang').is_file() or not (jdk / 'bin/java').is_file():
        parser.error('The PS5 SDK and host JDK must already be installed.')
    work.mkdir(parents=True, exist_ok=True)
    jdk_build = work / ('jdk-static-build' if args.static else 'jdk-build')
    # OpenJDK treats every file in CUSTOM_CONFIG_DIR as a configure dependency.
    # Keep build scripts/tests outside it so unrelated edits do not invalidate a build.
    custom_config = work / 'custom-config'
    custom_config.mkdir(exist_ok=True)
    hook = custom_config / 'custom-hook.m4'
    hook_text = (HERE / 'custom-hook.m4').read_text()
    if not hook.is_file() or hook.read_text() != hook_text:
        hook.write_text(hook_text)
    env = dict(os.environ, PS5_PAYLOAD_SDK=str(sdk),
               LLVM_CONFIG='/usr/bin/llvm-config-18',
               PATH=str(sdk / 'bin') + os.pathsep + os.environ['PATH'],
               CUSTOM_CONFIG_DIR=str(custom_config))

    def run(command, cwd, log):
        log = work / (('static-' if args.static else '') + log)
        print(shlex.join(map(str, command)), flush=True)
        with log.open('w') as output:
            result = subprocess.run(list(map(str, command)), cwd=cwd, env=env,
                                    stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print('\n'.join(log.read_text(errors='replace').splitlines()[-25:]))
            raise SystemExit(f'Failed; see {log}')

    for name, (url, digest) in SOURCES.items():
        source = work / name
        # Existing sources are retained so port patches can be developed incrementally.
        if not (source / 'configure').is_file():
            archive = work / (name + '.tar.gz')
            run(['curl', '-fL', '--retry', '2', '-o', archive, url], work, 'download.log')
            with archive.open('rb') as data:
                if hashlib.file_digest(data, 'sha256').hexdigest() != digest:
                    raise SystemExit(f'Source checksum mismatch: {archive}')
            source.mkdir(exist_ok=True)
            run(['tar', '-xzf', archive, '-C', source, '--strip-components=1'], work, 'extract.log')

    for source, patch in [('libffi', HERE / 'libffi-ps5.patch'),
                          ('openjdk21-bsd', HERE / 'openjdk-ps5.patch'),
                          ('openjdk21-bsd', HERE / 'openjdk-static.patch'),
                          ('openjdk21-bsd', HERE / 'openjdk-embedded.patch')]:
        if not patch_is_applied(work / source, patch):
            run(['patch', '--batch', '--forward', '--fuzz=0', '--dry-run', '-p1', '-i', patch], work / source, f'{source}-patch-check.log')
            run(['patch', '--batch', '--forward', '--fuzz=0', '-p1', '-i', patch], work / source, f'{source}-patch.log')

    (work / 'bin').mkdir(exist_ok=True)
    # Clang can pass --shared inside a response file. The SDK wrapper cannot see
    # that flag and adds -pie. Direct LLD handles response files and native ELF layout.
    linker = work / 'bin/prospero-lld'
    link_flags = ['-z', 'max-page-size=0x4000', '--eh-frame-hdr', '--hash-style=gnu',
                  '-mllvm', '-emulated-tls']
    for section in ('eh_frame', 'eh_frame_hdr'):
        link_flags.extend([f'--defsym=__{section}_start=ADDR(.{section})',
                           f'--defsym=__{section}_end=ADDR(.{section})+SIZEOF(.{section})'])
    write_script(linker, '#!/bin/bash\nexec /usr/lib/llvm-18/bin/ld.lld ' +
                 shlex.join(link_flags) + ' "$@"\n')
    for name, compiler in [('ps5-cc', 'prospero-clang'), ('ps5-cxx', 'prospero-clang++')]:
        wrapper = work / 'bin' / name
        # Use normal platform imports; never pull the SDK's payload startup into a JVM.
        write_script(wrapper, '#!/bin/bash\nexec ' + shlex.quote(str(sdk / 'bin' / compiler)) +
                           ' --start-no-unused-arguments -B' + shlex.quote(str(work / 'bin')) +
                           ' -nostartfiles -lkernel'
                           ' --end-no-unused-arguments "$@"\n')
    compilers = [f'CC={work}/bin/ps5-cc', f'CXX={work}/bin/ps5-cxx', f'AR={sdk}/bin/prospero-ar']

    if args.stage in ('all', 'ffi'):
        build = work / 'ffi-build'
        build.mkdir(exist_ok=True)
        run(['bash', work / 'libffi/configure', '--build=x86_64-pc-linux-gnu',
             '--host=x86_64-sie-freebsd', f'--prefix={work}/ffi-install', '--disable-shared',
             '--with-pic', '--disable-docs', '--disable-multi-os-directory',
             '--disable-exec-static-tramp', *compilers, f'RANLIB={sdk}/bin/prospero-ranlib'],
            build, 'configure-ffi.log')
        run(['make', f'-j{args.jobs}'], build, 'build-ffi.log')
        run(['make', 'install'], build, 'install-ffi.log')
        source = work / 'libffi'
        run(['clang-18', '-O2', '-D__PROSPERO__', '-DFFI_BUILDING', f'-I{build}',
             f'-I{build}/include', f'-I{source}/include', f'-I{source}/src',
             *[source / f'src/{f}' for f in ('types.c', 'prep_cif.c', 'x86/ffi64.c', 'x86/unix64.S')],
             HERE / 'ffi_check.c', '-o', work / 'ffi-check-host'], work, 'check-ffi-build.log')
        run([work / 'ffi-check-host'], work, 'check-ffi.log')
        print('PS5 libffi built; ABI regression check passed on host (console untested).')

    if args.stage in ('all', 'hotspot'):
        build = jdk_build
        build.mkdir(exist_ok=True)
        run(['bash', work / 'openjdk21-bsd/configure', '--build=x86_64-pc-linux-gnu',
             '--openjdk-target=x86_64-unknown-freebsd', '--with-toolchain-type=clang',
             f'--with-boot-jdk={jdk}', f'--with-build-jdk={jdk}', f'--with-sysroot={sdk}',
             '--with-jvm-variants=zero', '--enable-headless-only', f'--with-libffi={work}/ffi-install',
             '--with-jvm-features=-services,-jvmti', '--with-extra-ldflags=-Wl,-z,defs,--gc-sections',
             '--disable-warnings-as-errors', '--disable-precompiled-headers',
             '--with-debug-level=release', '--with-native-debug-symbols=none',
             '--with-zlib=bundled', '--with-freetype=bundled', *compilers,
             *(['--enable-static-build', '--with-extra-cflags=-DSTATIC_BUILD=1',
                '--with-extra-cxxflags=-DSTATIC_BUILD=1'] if args.static else []),
             f'NM={sdk}/bin/prospero-nm', f'STRIP={sdk}/bin/prospero-strip',
             'BUILD_CC=/usr/bin/clang-18', 'BUILD_CXX=/usr/bin/clang++-18'], build, 'configure-jdk.log')
        run(['make', 'hotspot', f'JOBS={args.jobs}'], build, 'build-jvm.log')
        library = build / ('support/modules_libs/java.base/zero/libjvm.' + ('a' if args.static else 'so'))
        if args.static:
            run(['llvm-ar-18', 't', library], work, 'jvm-static-members.log')
        else:
            run(['llvm-readelf-18', '-h', '-l', '-d', library], work, 'jvm-elf.log')
            run(['llvm-nm-18', '--undefined-only', library], work, 'jvm-imports.log')
        print('JVM core compilation finished; full Java runtime and console execution remain unverified.')

    if args.stage in ('all', 'base'):
        run(['python3', HERE / 'test_process_wait.py', work], work, 'check-process-wait.log')
        run(['make', 'java.base-java', 'java.base-libs', 'java.base-copy', 'java.base-gendata', f'JOBS={args.jobs}'],
            jdk_build, 'build-java-base.log')
        if args.static:
            from static_symbols import generate
            libdir = jdk_build / 'support/modules_libs/java.base'
            archives = [libdir / f'lib{name}.a' for name in ('java', 'net', 'nio', 'zip', 'jimage', 'verify')]
            archives.append(libdir / 'zero/libjvm.a')
            for name in ('java', 'net', 'nio', 'zip', 'verify'):
                symbols = subprocess.check_output(['llvm-nm-18', '--defined-only', '--just-symbol-name',
                                                   str(libdir / f'lib{name}.a')], text=True).splitlines()
                if f'JNI_OnLoad_{name}' not in symbols or 'JNI_OnLoad' in symbols:
                    raise SystemExit(f'{name}: static JNI load entry was not renamed')
            run(['python3', HERE / 'test_static_runtime.py'], work, 'check-static-runtime.log')
            count = generate(archives, work / 'static_symbols.inc')
            print(f'Generated {count} native references; static lookup regression passed on host.')
        print('Java base classes and libraries built; native application linking and console execution are separate checks.')


if __name__ == '__main__':
    main()
