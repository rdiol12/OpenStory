import struct, lz4.block
from PIL import Image
from nxls import NX

class NXB(NX):
    def __init__(self, path):
        super().__init__(path)
        bcount, boff = struct.unpack_from('<IQ', self.m, 28)
        self.btable = struct.unpack_from('<%dQ' % bcount, self.m, boff) if bcount else ()

    def bitmap(self, idx):
        name, child, nchild, t, data = self.node(idx)
        if t != 5:
            return None
        bid, w, h = struct.unpack('<IHH', data)
        off = self.btable[bid]
        clen = struct.unpack_from('<I', self.m, off)[0]
        raw = lz4.block.decompress(self.m[off+4:off+4+clen], uncompressed_size=w*h*4)
        img = Image.frombytes('RGBA', (w, h), raw)
        b, g, r, a = img.split()
        return Image.merge('RGBA', (r, g, b, a))

    def child(self, idx, name):
        n, child, nchild, t, d = self.node(idx)
        for c in range(child, child + nchild):
            if self.strings[self.node(c)[0]] == name:
                return c
        return None

    def vec(self, idx, name, default=(0, 0)):
        c = self.child(idx, name)
        if c is None:
            return default
        _, _, _, t, d = self.node(c)
        return struct.unpack('<ii', d) if t == 4 else default
