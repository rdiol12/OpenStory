#!/usr/bin/env python3
"""Check Home update identity and reject mislabeled selection audio."""
from pathlib import Path
import struct
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'platform/ps5'))
from presentation import update_uri, selection_audio

assert update_uri('http://library.test:3150', '01.000.002') == 'http://library.test:3150/api/v1/native-updates/PPSA99783/from/01.000.002/version.xml'
for origin in ('file:///tmp', 'https://user:password@library.test', 'https://library.test/wrong', 'http://library.test?token=abc'):
    try:
        update_uri(origin, '01.000.002')
    except ValueError:
        pass
    else:
        raise AssertionError(origin)
with tempfile.TemporaryDirectory() as directory:
    audio = Path(directory) / 'snd0.at9'
    fmt = bytearray(52)
    struct.pack_into('<HHI', fmt, 0, 0xfffe, 2, 48000)
    fmt[24:40] = bytes.fromhex('d242e147ba368d4d88fc61654f8c836c')
    body = b'WAVEfmt ' + struct.pack('<I', len(fmt)) + fmt + b'data' + struct.pack('<I', 4) + b'test'
    audio.write_bytes(b'RIFF' + struct.pack('<I', len(body)) + body)
    assert selection_audio(audio) == audio
    for data in (b'not audio', audio.read_bytes()[:-1], b'RIFF' + struct.pack('<I',len(body)) + body.replace(fmt[24:40],bytes(16))):
        audio.write_bytes(data)
        try:
            selection_audio(audio)
        except ValueError:
            pass
        else:
            raise AssertionError('Invalid ATRAC9 container accepted')
print('PASS: exact OpenStory update URI and bounded ATRAC9 input validation')
