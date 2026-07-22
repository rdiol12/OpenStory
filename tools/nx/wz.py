"""Minimal WZ (PKG1) reader: directory + img property trees + canvas decode.
Pure python (embedded AES-256 ECB for the GMS string keystream)."""
import struct, zlib
from PIL import Image

# ---- tiny AES-256 ECB (encrypt only) ----
SBOX = bytes.fromhex(
    '637c777bf26b6fc53001672bfed7ab76ca82c97dfa5947f0add4a2af9ca472c0'
    'b7fd9326363ff7cc34a5e5f171d8311504c723c31896059a071280e2eb27b275'
    '09832c1a1b6e5aa0523bd6b329e32f8453d100ed20fcb15b6acbbe394a4c58cf'
    'd0efaafb434d338545f9027f503c9fa851a3408f929d38f5bcb6da2110fff3d2'
    'cd0c13ec5f974417c4a77e3d645d197360814fdc222a908846eeb814de5e0bdb'
    'e0323a0a4906245cc2d3ac629195e479e7c8376d8dd54ea96c56f4ea657aae08'
    'ba78252e1ca6b4c6e8dd741f4bbd8b8a703eb5664803f60e613557b986c11d9e'
    'e1f8981169d98e949b1e87e9ce5528df8ca1890dbfe6426841992d0fb054bb16')
RCON = [0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36,0x6C,0xD8,0xAB,0x4D]

def xtime(a):
    a <<= 1
    return (a ^ 0x11B) & 0xFF if a & 0x100 else a

def aes256_expand(key):
    w = [list(key[i*4:i*4+4]) for i in range(8)]
    for i in range(8, 60):
        t = list(w[i-1])
        if i % 8 == 0:
            t = t[1:] + t[:1]
            t = [SBOX[b] for b in t]
            t[0] ^= RCON[i//8 - 1]
        elif i % 8 == 4:
            t = [SBOX[b] for b in t]
        w.append([w[i-8][j] ^ t[j] for j in range(4)])
    return w

def aes256_encrypt_block(w, block):
    s = [[block[r + 4*c] for c in range(4)] for r in range(4)]
    def addkey(rnd):
        for c in range(4):
            for r in range(4):
                s[r][c] ^= w[rnd*4 + c][r]
    addkey(0)
    for rnd in range(1, 15):
        for r in range(4):
            for c in range(4):
                s[r][c] = SBOX[s[r][c]]
        for r in range(1, 4):
            s[r] = s[r][r:] + s[r][:r]
        if rnd < 14:
            for c in range(4):
                a = [s[r][c] for r in range(4)]
                s[0][c] = xtime(a[0]) ^ xtime(a[1]) ^ a[1] ^ a[2] ^ a[3]
                s[1][c] = a[0] ^ xtime(a[1]) ^ xtime(a[2]) ^ a[2] ^ a[3]
                s[2][c] = a[0] ^ a[1] ^ xtime(a[2]) ^ xtime(a[3]) ^ a[3]
                s[3][c] = xtime(a[0]) ^ a[0] ^ a[1] ^ a[2] ^ xtime(a[3])
        addkey(rnd)
    return bytes(s[r][c] for c in range(4) for r in range(4))

WZ_KEY = bytes()
for b in (0x13, 0x08, 0x06, 0xB4, 0x1B, 0x0F, 0x33, 0x52):
    WZ_KEY += bytes((b, 0, 0, 0))

class Keystream:
    def __init__(self, iv4):
        self.buf = b''
        self.iv4 = iv4
        if any(iv4):
            self.w = aes256_expand(WZ_KEY)
            self.last = bytes(iv4) * 4
        else:
            self.w = None

    def get(self, n):
        if self.w is None:
            return bytes(n)
        while len(self.buf) < n:
            self.last = aes256_encrypt_block(self.w, self.last)
            self.buf += self.last
        return self.buf[:n]

def rotl32(v, n):
    n &= 31
    return ((v << n) | (v >> (32 - n))) & 0xFFFFFFFF

class WzFile:
    def __init__(self, path, iv4=(0x4D, 0x23, 0xC7, 0x2B)):
        self.d = open(path, 'rb').read()
        self.ks = Keystream(iv4)
        assert self.d[:4] == b'PKG1', 'not a wz file'
        self.fstart = struct.unpack_from('<I', self.d, 12)[0]
        encver = struct.unpack_from('<H', self.d, self.fstart)[0]
        self.hash = None
        for ver in range(1, 1024):
            h = 0
            for ch in str(ver):
                h = (32 * h + ord(ch) + 1) & 0xFFFFFFFF
            e = 0xFF ^ ((h >> 24) & 0xFF) ^ ((h >> 16) & 0xFF) ^ ((h >> 8) & 0xFF) ^ (h & 0xFF)
            if e == encver:
                # try parsing root dir with this hash
                self.hash = h
                try:
                    self.root = self.read_dir(self.fstart + 2)
                    if self.root:
                        self.version = ver
                        return
                except Exception:
                    pass
        raise RuntimeError('version not found')

    # --- primitives ---
    def cint(self, p):
        v = struct.unpack_from('<b', self.d, p)[0]
        if v == -128:
            return struct.unpack_from('<i', self.d, p + 1)[0], p + 5
        return v, p + 1

    def wzstring(self, p):
        ln = struct.unpack_from('<b', self.d, p)[0]
        p += 1
        if ln == 0:
            return '', p
        if ln < 0:  # ascii
            n = struct.unpack_from('<i', self.d, p)[0] if ln == -128 else -ln
            if ln == -128:
                p += 4
            raw = self.d[p:p+n]
            ks = self.ks.get(n)
            mask = 0xAA
            out = bytearray()
            for i in range(n):
                out.append(raw[i] ^ mask ^ ks[i])
                mask = (mask + 1) & 0xFF
            return out.decode('ascii', 'replace'), p + n
        else:  # utf16
            n = struct.unpack_from('<i', self.d, p)[0] if ln == 127 else ln
            if ln == 127:
                p += 4
            raw = self.d[p:p+n*2]
            ks = self.ks.get(n * 2)
            mask = 0xAAAA
            out = []
            for i in range(n):
                ch = struct.unpack_from('<H', raw, i*2)[0]
                kch = ks[i*2] | (ks[i*2+1] << 8)
                out.append(chr(ch ^ mask ^ kch))
                mask = (mask + 1) & 0xFFFF
            return ''.join(out), p + n*2

    def deref_string(self, p, img_base):
        """String or reference within an img."""
        t = self.d[p]
        p += 1
        if t in (0x00, 0x73):
            return self.wzstring(p)
        if t in (0x01, 0x1B):
            off = struct.unpack_from('<I', self.d, p)[0]
            s, _ = self.wzstring(img_base + off)
            return s, p + 4
        raise ValueError('bad string type 0x%02X at %d' % (t, p - 1))

    def offset(self, p):
        v = (p - self.fstart) ^ 0xFFFFFFFF
        v = (v * self.hash) & 0xFFFFFFFF
        v = (v - 0x581C3F6D) & 0xFFFFFFFF
        v = rotl32(v, v)
        v ^= struct.unpack_from('<I', self.d, p)[0]
        v = (v + self.fstart * 2) & 0xFFFFFFFF
        return v, p + 4

    # --- directory ---
    def read_dir(self, p):
        count, p = self.cint(p)
        out = {}
        if count < 0 or count > 10000:
            raise ValueError('bad dir count')
        for _ in range(count):
            t = self.d[p]
            p += 1
            if t == 2:
                soff = struct.unpack_from('<I', self.d, p)[0]
                p += 4
                t2 = self.d[self.fstart + soff]
                name, _ = self.wzstring(self.fstart + soff + 1)
                t = t2
            elif t in (3, 4):
                name, p = self.wzstring(p)
            elif t == 1:
                p += 10
                continue
            else:
                raise ValueError('bad dir entry %d' % t)
            size, p = self.cint(p)
            _, p = self.cint(p)
            off, p = self.offset(p)
            out[name] = (t, off, size)
        return out

    def open_img(self, dirpath):
        node = self.root
        for part in dirpath[:-1]:
            t, off, size = node[part]
            node = self.read_dir(off)
        t, off, size = node[dirpath[-1]]
        return WzImg(self, off)

class WzImg:
    def __init__(self, wz, base):
        self.wz = wz
        self.base = base

    def parse(self, p=None, depth=0, maxdepth=99):
        wz = self.wz
        if p is None:
            p = self.base
        t, _ = wz.deref_string(p, self.base)
        p_after = None
        # root object: expects "Property"
        return self.parse_obj(p)

    def parse_obj(self, p, depth=0):
        wz = self.wz
        typ, p = wz.deref_string(p, self.base)
        if typ == 'Property':
            p += 2  # reserved
            return self.parse_proplist(p, depth)
        if typ == 'Canvas':
            return self.parse_canvas(p, depth)
        if typ == 'Shape2D#Vector2D':
            x, p = wz.cint(p)
            y, p = wz.cint(p)
            return {'__type': 'vector', 'x': x, 'y': y}, p
        if typ == 'Shape2D#Convex2D':
            n, p = wz.cint(p)
            items = {}
            for i in range(n):
                v, p = self.parse_obj(p, depth + 1)
                items[str(i)] = v
            return items, p
        if typ == 'Sound_DX8':
            p += 1
            size, p = wz.cint(p)
            _, p = wz.cint(p)
            return {'__type': 'sound'}, p + 51 + size
        if typ == 'UOL':
            p += 1
            s, p = wz.deref_string(p, self.base)
            return {'__type': 'uol', 'path': s}, p
        raise ValueError('unknown obj type %r' % typ)

    def parse_proplist(self, p, depth):
        wz = self.wz
        count, p = wz.cint(p)
        out = {}
        for _ in range(count):
            name, p = wz.deref_string(p, self.base)
            t = wz.d[p]
            p += 1
            if t == 0:
                out[name] = None
            elif t in (2, 11):
                out[name] = struct.unpack_from('<H', wz.d, p)[0]; p += 2
            elif t in (3, 19):
                v, p = wz.cint(p); out[name] = v
            elif t == 20:
                v = struct.unpack_from('<b', wz.d, p)[0]; p += 1
                if v == -128:
                    v = struct.unpack_from('<q', wz.d, p)[0]; p += 8
                out[name] = v
            elif t == 4:
                if wz.d[p] == 0x80:
                    out[name] = struct.unpack_from('<f', wz.d, p+1)[0]; p += 5
                else:
                    out[name] = 0.0; p += 1
            elif t == 5:
                out[name] = struct.unpack_from('<d', wz.d, p)[0]; p += 8
            elif t == 8:
                s, p = wz.deref_string(p, self.base); out[name] = s
            elif t == 9:
                size = struct.unpack_from('<I', wz.d, p)[0]; p += 4
                v, _ = self.parse_obj(p, depth + 1)
                out[name] = v
                p += size
            else:
                raise ValueError('bad prop type %d for %r' % (t, name))
        return out, p

    def parse_canvas(self, p, depth):
        wz = self.wz
        p += 1
        haskids = wz.d[p]
        p += 1
        kids = {}
        if haskids:
            p += 2
            kids, p = self.parse_proplist(p, depth + 1)
        w, p = wz.cint(p)
        h, p = wz.cint(p)
        fmt, p = wz.cint(p)
        fmt2 = wz.d[p]; p += 1
        p += 4
        dlen = struct.unpack_from('<I', wz.d, p)[0]; p += 4
        p += 1
        data = wz.d[p:p + dlen - 1]
        out = dict(kids)
        out['__type'] = 'canvas'
        out['__w'], out['__h'], out['__fmt'], out['__fmt2'] = w, h, fmt, fmt2
        out['__data'] = data
        return out, p + dlen - 1

    def decode_canvas(self, c):
        wz = self.wz
        data = c['__data']
        w, h, fmt, fmt2 = c['__w'], c['__h'], c['__fmt'], c['__fmt2']
        if len(data) >= 2 and data[0] == 0x78:
            raw = zlib.decompressobj().decompress(data)
        else:
            # keystream-xored chunks
            out = bytearray()
            i = 0
            while i + 4 <= len(data):
                bl = struct.unpack_from('<I', data, i)[0]
                i += 4
                ks = wz.ks.get(bl)
                out += bytes(a ^ b for a, b in zip(data[i:i+bl], ks))
                i += bl
            raw = zlib.decompressobj().decompress(bytes(out))
        scale = 1 << fmt2 if fmt2 else 1
        sw, sh = w // scale, h // scale
        if fmt == 2:
            img = Image.frombytes('RGBA', (sw, sh), bytes(raw[:sw*sh*4]))
            b, g, r, a = img.split()
            img = Image.merge('RGBA', (r, g, b, a))
        elif fmt == 1:
            px = bytearray(sw * sh * 4)
            for i in range(sw * sh):
                v = raw[i*2] | (raw[i*2+1] << 8)
                b = (v & 0xF) * 17
                g = ((v >> 4) & 0xF) * 17
                r = ((v >> 8) & 0xF) * 17
                a = ((v >> 12) & 0xF) * 17
                px[i*4:i*4+4] = bytes((r, g, b, a))
            img = Image.frombytes('RGBA', (sw, sh), bytes(px))
        elif fmt == 513:
            px = bytearray(sw * sh * 4)
            for i in range(sw * sh):
                v = raw[i*2] | (raw[i*2+1] << 8)
                r = ((v >> 11) & 31) << 3
                g = ((v >> 5) & 63) << 2
                b = (v & 31) << 3
                px[i*4:i*4+4] = bytes((r, g, b, 255))
            img = Image.frombytes('RGBA', (sw, sh), bytes(px))
        else:
            raise ValueError('canvas fmt %d' % fmt)
        if scale > 1:
            img = img.resize((w, h), Image.NEAREST)
        return img
