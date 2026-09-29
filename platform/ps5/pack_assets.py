#!/usr/bin/env python3
"""Store NX data with a reversible wrapper so damaged reference blocks are not reused."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

MAGIC = b'OSNX\x01\0\0\0'
TRANSLATE = bytes(value ^ 0xA5 for value in range(256))


def pack(source, destination):
    original_hash, packaged_hash = hashlib.sha256(), hashlib.sha256()
    with source.open('rb') as src:
        if src.read(4) != b'PKG4' or source.stat().st_size < 52:
            raise ValueError('Expected an original NX file: ' + str(source))
        src.seek(0)
        header = struct.pack('<8sQ', MAGIC, source.stat().st_size)
        with destination.open('xb') as dst:
            dst.write(header)
            packaged_hash.update(header)
            while data := src.read(1024 * 1024):
                original_hash.update(data)
                stored = data.translate(TRANSLATE)
                dst.write(stored)
                packaged_hash.update(stored)
    return dict(originalBytes=source.stat().st_size, originalSha256=original_hash.hexdigest(),
                preparedBytes=destination.stat().st_size, preparedSha256=packaged_hash.hexdigest())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    files = sorted(args.source.glob('*.nx'))
    if not files:
        raise ValueError('No NX files found')
    report = {'format': 'OSNX version 1: 16-byte header, NX bytes XOR 0xA5', 'files': {}}
    for source in files:
        report['files'][source.name] = pack(source, args.output / source.name)
        print('Prepared', source.name, flush=True)
    args.manifest.write_text(json.dumps(report, indent=2) + '\n')
