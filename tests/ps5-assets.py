#!/usr/bin/env python3
"""Compare packaged game payloads with staged input; integrity alone missed damaged NX data."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


p = argparse.ArgumentParser(description=__doc__)
p.add_argument('package', type=Path)
p.add_argument('source', type=Path)
p.add_argument('publisher', type=Path)
p.add_argument('output', type=Path, help='New scratch/report directory, preferably on the build drive')
p.add_argument('files', nargs='*', help='Optional relative payload paths; default checks all NX files, executable, runtime and Settings')
args = p.parse_args()
source = args.source.resolve()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=False)
files = args.files or ['eboot.bin', 'Settings', 'sce_module/libc.prx'] + [
    path.relative_to(source).as_posix() for path in sorted((source / 'wz').glob('*.nx'))]
result = {'passed': False, 'package': str(args.package.resolve()), 'files': {}}
for index, name in enumerate(files):
    original = (source / name).resolve()
    if not original.is_relative_to(source) or not original.is_file():
        raise ValueError('Not a staged payload: ' + name)
    extracted = output / ('payload-' + str(index))
    command = [str(args.publisher.resolve()), 'img_extract', '--passcode', '0' * 32,
               '--no_progress_bar', str(args.package.resolve()) + ':' + name, str(extracted)]
    with (output / ('extract-' + str(index) + '.log')).open('wb') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    expected, actual = sha256(original), sha256(extracted)
    matched = expected == actual and original.stat().st_size == extracted.stat().st_size
    result['files'][name] = dict(size=original.stat().st_size, expectedSha256=expected,
                               actualSha256=actual, matches=matched)
    (output / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')
    if not matched:
        raise SystemExit('FAIL: packaged ' + name + ' differs; extracted bytes preserved at ' + str(extracted))
    extracted.unlink()  # Only this newly extracted, verified scratch file.
    print('PASS:', name, flush=True)
result['passed'] = True
(output / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: all', len(files), 'packaged payloads match staged input')
