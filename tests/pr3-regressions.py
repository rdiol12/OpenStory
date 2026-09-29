#!/usr/bin/env python3
"""Run valid-input PR3 regressions against a completed SDL host build, offline.

Usage: python3 tests/pr3-regressions.py [build/sdl-host] [directory-with-NX-assets]
Requires the normal desktop display; uses dummy audio and temporary settings.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', nargs='?', type=Path, default=root / 'build/sdl-host')
parser.add_argument('assets', nargs='?', type=Path, default=root / 'build/ps5-update-01.000.028/host-assets')
args = parser.parse_args()
target = args.build.resolve() / 'platform/sdl'
metadata = target / 'CMakeFiles/OpenStory.dir'
flags = dict(line.split(' = ', 1) for line in (metadata / 'flags.make').read_text().splitlines() if ' = ' in line)
link = shlex.split((metadata / 'link.txt').read_text())
main_object = next(token for token in link if token.endswith('/src/MapleStory.cpp.o'))

with tempfile.TemporaryDirectory(prefix='openstory-pr3-regressions-') as directory:
    work = Path(directory)
    for asset in args.assets.resolve().glob('*.nx'):
        (work / asset.name).symlink_to(asset)
    assert (work / 'Map.nx').exists() and (work / 'UI.nx').exists(), 'v83 NX assets are required'
    (work / 'Settings').write_text('OfflinePreview = true\nAutoLogin = false\nFullscreen = false\n')
    test_object = work / 'test.o'
    subprocess.run([link[0], *shlex.split(' '.join(flags[key] for key in ('CXX_DEFINES', 'CXX_INCLUDES', 'CXX_FLAGS'))),
                    '-UNDEBUG', '-c', str(Path(__file__).with_suffix('.cpp')), '-o', str(test_object)], check=True)
    renamed_main = work / 'MapleStory.o'
    subprocess.run(['objcopy', '--redefine-sym', 'main=openstory_original_main',
                    str(target / main_object), str(renamed_main)], check=True)
    link = [str(renamed_main) if token == main_object else token for token in link
            if not token.startswith('-Wl,--dependency-file=')]
    binary = work / 'pr3-regressions'
    link[link.index('-o') + 1] = str(binary)
    link[1:1] = [str(test_object), '-Wl,--wrap=_ZN2ms6Combat8use_moveEi']
    subprocess.run(link, cwd=target, check=True)
    result = subprocess.run([binary], cwd=work,
                            env=dict(os.environ, SDL_AUDIODRIVER='dummy'), timeout=120)
    raise SystemExit(result.returncode)
