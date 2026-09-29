"""Normalize OpenStory's plain native build outputs for the package format checker."""
import struct


def normalize(data):
    def number(fmt, offset):
        if offset < 0 or offset + struct.calcsize(fmt) > len(data):
            raise ValueError('Truncated native container')
        return struct.unpack_from(fmt, data, offset)[0]

    if number('<I', 0) not in (0x1d3d154f, 0xeef51454):
        raise ValueError('Expected a native build container')
    count, declared = number('<H', 0x18), number('<Q', 0x10)
    elf = 0x20 + count * 0x20
    if not count or data[elf:elf+4] != b'\x7fELF' or declared > len(data):
        raise ValueError('Invalid native container header')
    tail = 0
    for index in range(count):
        at = 0x20 + index * 0x20
        flags, offset, size = (number('<Q', at + n) for n in (0, 8, 16))
        if flags & 0xa or offset < elf or offset + size > declared:
            raise ValueError('Expected bounded, unencrypted, uncompressed build segments')
        tail = max(tail, offset + size)
    if declared != ((tail + 15) & ~15):
        raise ValueError('Unexpected native container alignment')
    if number('<Q', elf + 0x20) != 0x40 or number('<H', elf + 0x36) != 0x38:
        raise ValueError('Unexpected ELF program-header layout')
    records = [number('<Q', elf + 0x40 + i * 0x38 + 0x20)
               for i in range(number('<H', elf + 0x38))
               if number('<I', elf + 0x40 + i * 0x38) == 0x6fffff01]
    if len(records) != 1 or not records[0]:
        raise ValueError('Missing native SDK version records')
    size = records[0]
    if len(data) == declared + size and not any(data[tail:declared]):
        result = bytearray(data)
    elif len(data) == max(declared, tail + size):
        result = bytearray(data[:tail] + bytes(declared - tail) + data[tail:tail+size])
    else:
        raise ValueError('Truncated or unexpected SDK version records')
    # Fix8 lacks fix9's format normalization. No executable segment or SDK value changes.
    struct.pack_into('<I', result, 0, 0xeef51454)
    return bytes(result)
