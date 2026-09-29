"""Validate the native Home-menu build inputs before compiling or packaging."""
from pathlib import Path
import struct
from urllib.parse import urlsplit


def update_uri(origin, version):
    url = urlsplit(origin)
    if (url.scheme not in ('http', 'https') or not url.hostname or url.username or
            url.password or url.query or url.fragment or url.path not in ('', '/')):
        raise ValueError('Update origin must be an HTTP(S) server origin without credentials or a path')
    _ = url.port  # Reject malformed/out-of-range ports as well.
    return f'{url.scheme}://{url.netloc}/api/v1/native-updates/PPSA99783/from/{version}/version.xml'


def selection_audio(value):
    file = Path(value)
    if file.is_symlink() or not file.is_file() or not 44 <= file.stat().st_size <= 16 * 1024 * 1024:
        raise ValueError('Selection audio must be a regular ATRAC9 file no larger than 16 MiB')
    data = file.read_bytes()
    if data[:4] != b'RIFF' or data[8:12] != b'WAVE' or struct.unpack_from('<I', data, 4)[0] != len(data) - 8:
        raise ValueError('Selection audio has an invalid RIFF/WAVE container')
    offset, atrac9, samples = 12, False, False
    while offset + 8 <= len(data):
        kind, size = struct.unpack_from('<4sI', data, offset)
        start, end = offset + 8, offset + 8 + size
        if end > len(data):
            raise ValueError('Selection audio contains a truncated chunk')
        if kind == b'fmt ':
            atrac9 = (size >= 40 and struct.unpack_from('<HHI', data, start) == (0xfffe, 2, 48000)
                      and data[start+24:start+40] == bytes.fromhex('d242e147ba368d4d88fc61654f8c836c'))
        if kind == b'data':
            samples = size > 0
        offset = end + (size & 1)
    if not atrac9 or not samples or offset != len(data):
        raise ValueError('Selection audio must contain 48-kHz stereo ATRAC9 data')
    return file
