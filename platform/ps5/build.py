#!/usr/bin/env python3
"""Build OpenStory using the public native runtime already in PS5Library (WSL/Linux)."""
import argparse
import hashlib
import ipaddress
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
from presentation import update_uri, selection_audio
from startup import instrument
from opengl import build_presenter

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--ps5library', type=Path, default=ROOT.parent / 'PS5Library')
p.add_argument('--sdk', type=Path, default=ROOT / 'build/deps/ps5-payload-sdk')
p.add_argument('--opengl', type=Path, default=ROOT / 'build/deps/ps5-opengl-sdk-0.3.0')
p.add_argument('--jobs', default='12')
p.add_argument('--build-dir', type=Path, default=ROOT / 'build/ps5')
p.add_argument('--log-host', type=ipaddress.IPv4Address, help='Send app diagnostics to this PC on UDP 9978')
p.add_argument('--version', help='Content version, for example 01.000.002')
p.add_argument('--update-origin', help='Local Library server origin for Home-menu updates')
p.add_argument('--selection-audio', type=Path, help='48-kHz stereo ATRAC9 Home-menu music')
args = p.parse_args()
version = args.version or ('01.000.001' if args.log_host else '01.000.000')
if not re.fullmatch(r'[0-9]{2}\.[0-9]{3}\.[0-9]{3}', version):
    p.error('--version must have the form 01.000.002')
try:
    version_uri = update_uri(args.update_origin, version) if args.update_origin else None
    home_audio = selection_audio(args.selection_audio) if args.selection_audio else None
except ValueError as error:
    p.error(str(error))
build = args.build_dir.resolve()
library, sdk, opengl = (x.resolve() for x in (args.ps5library, args.sdk, args.opengl))
native = library / 'upstream/ProsperoTV/tooling/native'
work = build / 'native'
work.mkdir(parents=True, exist_ok=True)
(work / 'openstory-version.h').write_text('#define OPENSTORY_CONTENT_VERSION "' + version + '"\n')

def run(*command):
    subprocess.run([str(x) for x in command], check=True, cwd=ROOT)

for name, source in [('app_crt.cpp', native / 'app_crt.cpp'),
                     ('app_cpp_runtime.cpp', library / 'ps5/build/native-app/app_cpp_runtime.cpp')]:
    shutil.copyfile(source, work / name)
if args.log_host:
    startup = work / 'app_crt.cpp'
    startup.write_text(instrument(startup.read_text()))
layout = (native / 'ps5-pie.ld').read_text()
for section in ('eh_frame_hdr', 'eh_frame'):
    marker = 'KEEP(*(.' + section + '))'
    if layout.count(marker) != 1:
        raise ValueError('Unexpected native linker layout')
    layout = layout.replace(marker, '__' + section + '_start = .; ' + marker + ' __' + section + '_end = .;')
(work / 'native.ld').write_text(layout)
os.environ['PS5_PAYLOAD_SDK'] = str(sdk)
# SDK v0.43 omits CommonDialog; use the same link-only import facade as PS5Library.
# The installed application imports the console module, never this build-time file.
run(sdk / 'bin/prospero-clang++', '-std=c++20', '-O2', '-fPIC', '-c',
    native / 'ps5_radio_import_stub_common_dialog.cpp', '-o', work / 'common-dialog.o')
run(sdk / 'bin/prospero-lld', '--shared', '-soname', 'libSceCommonDialog.sprx',
    '-o', work / 'libSceCommonDialog.so', work / 'common-dialog.o')
presenter = build_presenter(sdk, opengl, work / 'opengl-presenter')
run('cmake', '-S', ROOT, '-B', build, '-DOPENSTORY_SDL=ON',
    '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY', '-DCMAKE_VERBOSE_MAKEFILE=OFF',
    '-DCMAKE_TOOLCHAIN_FILE=' + str(sdk / 'toolchain/prospero.cmake'),
    '-DPS5OpenGL_DIR=' + str(opengl / 'sdk/lib/cmake/PS5OpenGL'),
    '-DPS5_NATIVE_DIR=' + str(work), '-DPS5_LIBRARY_DIR=' + str(library),
    '-DOPENSTORY_OPENGL_PRESENTER=' + str(presenter),
    '-DOPENSTORY_LOG_HOST=' + (str(args.log_host) if args.log_host else ''),
    '-DOPENSTORY_LOG_ADDRESS=' + str(int(args.log_host) if args.log_host else 0))
run('cmake', '--build', build, '-j', args.jobs)
symbols = subprocess.check_output(['llvm-nm-18', str(build / 'platform/sdl/OpenStory.elf')], text=True)
assert any(line.split() == ['U', 'sceCommonDialogInitialize'] for line in symbols.splitlines()), 'CommonDialog must remain a real console import'
if 'kernel_mprotect' in symbols or 'kernel_get_fw_version' in symbols:
    raise ValueError('Native title must use platform imports, not payload runtime helpers')
for name in ('aligned_alloc', 'strdup', 'strndup', 'asprintf', 'vasprintf'):
    if any(line.split() == ['U', name] for line in symbols.splitlines()):
        raise ValueError('Allocation must use the executable heap: ' + name)
tool = work / 'ps5-native-tool'
run('clang++-18', '-std=c++20', '-O2',
    *[native / x for x in ('native_app_builder.cpp', 'self_container.cpp', 'elf_object.cpp', 'sce_module_writer.cpp')],
    '-lz', '-o', tool)
run(tool, 'link', '--in', build / 'platform/sdl/OpenStory.elf', '--out', work / 'eboot.elf',
    '--stub-dir', sdk / 'target/lib', '--stub', opengl / 'sdk/lib/libSceAgc.so',
    '--stub', work / 'libSceCommonDialog.so',
    '--stub', opengl / 'sdk/lib/libSceAgcDriver.so', '--module-sdk', '0x02000009',
    '--companion-sdk', '0x08050001', '--file-name', 'eboot.elf')
dist = build / 'dist/PPSA99783'
(dist / 'sce_sys').mkdir(parents=True, exist_ok=True)
(dist / 'sce_module').mkdir(exist_ok=True)
run(tool, 'self', '--sign', '--in', work / 'eboot.elf', '--out', dist / 'eboot.bin', '--magic', '0x1D3D154F')
run(tool, 'self', '--extract', '--file', dist / 'eboot.bin', '--out', work / 'readback.elf')
# Reuse PS5Library's executable-segment verification, not just a file extension check.
import sys
sys.path.insert(0, str(library / 'ps5/native'))
from verify import verify_readback
verify_readback((work / 'eboot.elf').read_bytes(), (work / 'readback.elf').read_bytes())
runtime = library / 'ps5/build/native-app/dist/PPSA99051/sce_module/libc.prx'
shutil.copyfile(runtime, dist / 'sce_module/libc.prx')
param = {'titleId': 'PPSA99783', 'conceptId': '99783', 'contentId': 'UP9000-PPSA99783_00-OPENSTORYPS50000',
    'contentVersion': version, 'masterVersion': '01.00', 'applicationCategoryType': 0,
    'applicationDrmType': 'standard', 'contentBadgeType': 1, 'downloadDataSize': 256,
    'attribute': 0, 'attribute2': 0, 'attribute3': 0, 'ageLevel': {'default': 0, 'US': 0},
    'requiredSystemSoftwareVersion': '0x0200000000000000', 'sdkVersion': '0x0200000000000000',
    'localizedParameters': {'defaultLanguage': 'en-US', 'en-US': {'titleName': 'OpenStory'}}}
if version_uri:
    param['versionFileUri'] = version_uri
(dist / 'sce_sys/param.json').write_text(json.dumps(param, indent=2) + '\n')
if home_audio:
    shutil.copyfile(home_audio, dist / 'sce_sys/snd0.at9')
for name in ('icon0.png', 'pic0.png', 'pic1.png', 'pic2.png'):
    shutil.copyfile(ROOT / 'platform/ps5' / name, dist / 'sce_sys' / name)
manifest = {'hardwareTested': False, 'packageBuilt': False, 'contentVersion': version,
    'diagnosticLogHost': str(args.log_host) if args.log_host else None,
    'versionFileUri': version_uri,
    'ebootSha256': hashlib.sha256((dist / 'eboot.bin').read_bytes()).hexdigest(),
    'runtimeSha256': hashlib.sha256(runtime.read_bytes()).hexdigest(),
    'nativeSourceRevision': subprocess.check_output(['git', '-C', str(native), 'rev-parse', 'HEAD'], text=True).strip()}
(build / 'build.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('Native app folder:', dist)
