#!/usr/bin/env python3
"""Relink completed host objects and draw controls against the original v83 UI.nx."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', type=Path)
parser.add_argument('assets', type=Path)
parser.add_argument('cases', nargs='*', default=['slider', 'combo', 'game'])
args = parser.parse_args()
target = args.build.resolve() / 'platform/sdl'
metadata = target / 'CMakeFiles/OpenStory.dir'
flags = dict(line.split(' = ', 1) for line in (metadata / 'flags.make').read_text().splitlines() if ' = ' in line)
link = shlex.split((metadata / 'link.txt').read_text())
main_object = next(token for token in link if token.endswith('/src/MapleStory.cpp.o'))
with tempfile.TemporaryDirectory(prefix='openstory-v83-ui-') as directory:
    work = Path(directory)
    for asset in args.assets.resolve().glob('*.nx'):
        (work / asset.name).symlink_to(asset)
    assert (work / 'UI.nx').exists() and not (work / 'UI_83.nx').exists()
    (work / 'Settings').write_text('OfflinePreview = true\nAutoLogin = false\nFullscreen = false\n')
    test_object = work / 'test.o'
    subprocess.run([link[0], *shlex.split(' '.join(flags[key] for key in ('CXX_DEFINES', 'CXX_INCLUDES', 'CXX_FLAGS'))),
                    '-UNDEBUG', '-c', str(Path(__file__).with_suffix('.cpp').resolve()), '-o', str(test_object)], check=True)
    renamed_main = work / 'MapleStory.o'
    subprocess.run(['objcopy', '--redefine-sym', 'main=openstory_original_main',
                    str(target / main_object), str(renamed_main)], check=True)
    link = [str(renamed_main) if token == main_object else token for token in link
            if not token.startswith('-Wl,--dependency-file=')]
    binary = work / 'v83-ui'
    link[link.index('-o') + 1] = str(binary)
    link.insert(1, str(test_object))
    subprocess.run(link, cwd=target, check=True)
    failures = []
    for case in args.cases:
        try:
            subprocess.run([binary, case], cwd=work, env=dict(os.environ, SDL_AUDIODRIVER='dummy'),
                           check=True, timeout=20)
        except (subprocess.TimeoutExpired, subprocess.CalledProcessError) as error:
            failures.append(case)
            print(f'FAIL {case}: {error}', flush=True)
    assert not failures, f'v83 UI failures: {failures}'
