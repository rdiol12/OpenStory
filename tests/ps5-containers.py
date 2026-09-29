"""Check the native builder's version-record padding using a tiny plain container."""
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'platform/ps5'))
from containers import normalize

data = bytearray(0x104)
struct.pack_into('<I', data, 0, 0x1d3d154f)
struct.pack_into('<Q', data, 0x10, 0x100)
struct.pack_into('<H', data, 0x18, 1)
struct.pack_into('<QQQQ', data, 0x20, 0x2804, 0xf0, 8, 8)
data[0x40:0x44] = b'\x7fELF'
struct.pack_into('<Q', data, 0x60, 0x40)
struct.pack_into('<HH', data, 0x76, 0x38, 1)
struct.pack_into('<I', data, 0x80, 0x6fffff01)
struct.pack_into('<Q', data, 0xa0, 12)
data[0xf0:0xf8] = b'PAYLOAD!'
data[0xf8:] = b'VERSION-DATA'
fixed = normalize(bytes(data))
assert fixed[:4] == bytes.fromhex('5414f5ee')
assert fixed[4:0xf8] == data[4:0xf8]
assert fixed[0xf8:0x100] == bytes(8)
assert fixed[0x100:] == b'VERSION-DATA'
assert normalize(fixed) == fixed
for broken in (bytes(data[:80]), bytes(data[:-1]), b'not a container'):
    try:
        normalize(broken)
    except ValueError:
        pass
    else:
        raise AssertionError('Malformed container accepted')
print('PASS: version records aligned; payload preserved; normalization idempotent')
