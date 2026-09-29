#!/usr/bin/env python3
"""Build and verify the native OpenStory package on Windows using the local tooling."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from containers import normalize

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--toolkit', type=Path, required=True)
p.add_argument('--ps5library', type=Path, default=ROOT.parent / 'PS5Library')
p.add_argument('--build-dir', type=Path, default=ROOT / 'build/ps5')
args = p.parse_args()
toolkit, library = args.toolkit.resolve(), args.ps5library.resolve()
publisher = toolkit / 'toolchain/prospero-pub-cmd.exe'
build = args.build_dir.resolve()
source = build / 'dist/PPSA99783'
work = Path(tempfile.mkdtemp(prefix='package-', dir=build))
app = work / 'app'
app.mkdir()

def run(log_name, *command, check=True):
    with (work / log_name).open('w', encoding='utf-8') as log:
        result = subprocess.run([str(x) for x in command], cwd=ROOT,
            stdout=log, stderr=subprocess.STDOUT)
    if check and result.returncode:
        raise RuntimeError(f'{log_name} failed: see {work / log_name}')
    return result.returncode

# Prepare small packaging inputs first. The publisher supplies the native textures
# and this title's own debug license files; no metadata from another title is used.
for file in source.rglob('*'):
    if file.is_symlink():
        raise ValueError(f'Symlinks are not package inputs: {file}')
    if file.is_file() and file.relative_to(source).parts[0] != 'wz':
        target = app / file.relative_to(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(file, target)
for name in ('icon0.png', 'pic0.png', 'pic1.png', 'pic2.png'):
    shutil.copyfile(ROOT / 'platform/ps5' / name, app / 'sce_sys' / name)
param_path = app / 'sce_sys/param.json'
param = json.loads(param_path.read_text())
param['applicationDrmType'] = 'standard'
param_path.write_text(json.dumps(param, indent=2) + '\n')
for name in ('eboot.bin', 'sce_module/libc.prx'):
    target = app / name
    target.write_bytes(normalize(target.read_bytes()))
gp5 = work / 'resources.gp5'
run('resources-project.log', sys.executable, toolkit / 'scripts/create-gp5-from-folder.py',
    app, gp5, '--absolute-paths', '--keep-keystone')
resource_package = work / 'resources.pkg'
run('resources-build.log', publisher, 'img_create', '--oformat', 'nwonly',
    '--compression_level', '3', gp5, resource_package)
for name in ('icon0.dds', 'pic0.dds', 'pic1.dds', 'pic2.dds', 'license.dat', 'license.info'):
    target = app / 'sce_sys' / name
    if target.exists():
        target.unlink()  # Only this build's disposable copy, never the source.
    run('extract-' + name + '.log', publisher, 'img_extract', '--passcode', '0' * 32,
        '--no_progress_bar', str(resource_package) + ':sce_sys/' + name, target)

# Reuse the helper's container alignment repair and its English-only PlayGo data.
# Version records retain the executable's actual SDK; no firmware floor is lowered.
for entry in ET.parse(gp5).getroot().find('files'):
    name = entry.get('dst_path')
    if name in ('eboot.bin', 'sce_module/libc.prx', 'sce_sys/playgo-scenario.json'):
        supplied = Path(entry.get('src_path'))
        target = app / name
        if supplied.resolve() != target.resolve():
            shutil.copyfile(supplied, target)
assets = source / 'wz'
if not assets.is_dir() or not list(assets.glob('*.nx')):
    raise ValueError('Missing client NX files in ' + str(assets))
for file in assets.rglob('*'):
    if file.is_symlink():
        raise ValueError(f'Symlinks are not package inputs: {file}')
    if file.is_file():
        target = app / 'wz' / file.relative_to(assets)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.hardlink_to(file)  # Read-only build inputs; avoid copying 4.5 GB.

run('package-build.log', 'dotnet', 'run', '--project', ROOT / 'platform/ps5/package/Package.csproj',
    '--configuration', 'Release', '--artifacts-path', build / 'package-tools',
    '-p:PS5LibraryRoot=' + str(library), '--', app, work / 'output')
packages = list((work / 'output').glob('*.pkg'))
if len(packages) != 1:
    raise ValueError('Expected exactly one completed package')
package = packages[0]
format_ok = run('package-format.log', publisher, 'img_verify', '--passcode', '0' * 32,
    '--format_check', 'on', '--integrity_check', 'off', '--no_progress_bar', package, check=False) == 0
integrity_ok = run('package-integrity.log', publisher, 'img_verify', '--passcode', '0' * 32,
    '--format_check', 'off', '--integrity_check', 'on', '--no_progress_bar', package, check=False) == 0
if not integrity_ok:
    raise RuntimeError('Package integrity failed; previous output preserved. See ' + str(work))

output = build / 'OpenStory-PS5.pkg'
package.replace(output)
manifest_path = build / 'build.json'
manifest = json.loads(manifest_path.read_text())
with output.open('rb') as stream:
    digest = hashlib.file_digest(stream, 'sha256').hexdigest()
manifest.update(titleId=param['titleId'], packageBuilt=True, packageSha256=digest, packageBytes=output.stat().st_size,
    packageFormatPassed=format_ok, packageIntegrityPassed=integrity_ok,
    installationTested=False, hardwareTested=False, packageProfile='LibProsperoPkg debug image',
    packageBuildDirectory=str(work),
    packagedEbootSha256=hashlib.sha256((app / 'eboot.bin').read_bytes()).hexdigest(),
    packagedRuntimeSha256=hashlib.sha256((app / 'sce_module/libc.prx').read_bytes()).hexdigest(),
    packagerMetadataSha256=hashlib.sha256((library / 'upstream/LibProsperoPKG-drakmor/src/LibProsperoPkg/PFS/ProsperoPs5InnerMetadata.cs').read_bytes()).hexdigest(),
    packagerRevision=subprocess.check_output(['git', '-C', str(library / 'upstream/LibProsperoPKG-drakmor'),
        'rev-parse', 'HEAD'], text=True).strip())
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
for name in ('package-format.log', 'package-integrity.log', 'package-build.log'):
    shutil.copyfile(work / name, build / name)
print('Package:', output)
print('Integrity:', 'PASS' if integrity_ok else 'FAIL')
print('Format:', 'PASS' if format_ok else 'FAIL')
if not format_ok:
    for line in (work / 'package-format.log').read_text(errors='replace').splitlines():
        if '[Error]' in line:
            print(line)
raise SystemExit(0 if format_ok else 1)
