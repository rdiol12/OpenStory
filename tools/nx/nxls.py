import struct, sys, mmap

class NX:
    def __init__(self, path):
        self.f = open(path, 'rb')
        self.m = mmap.mmap(self.f.fileno(), 0, access=mmap.ACCESS_READ)
        magic, ncount, noff, scount, soff = struct.unpack_from('<4sIQIQ', self.m, 0)
        assert magic == b'PKG4', magic
        self.ncount, self.noff = ncount, noff
        stable = struct.unpack_from('<%dQ' % scount, self.m, soff)
        self.strings = []
        for off in stable:
            ln = struct.unpack_from('<H', self.m, off)[0]
            self.strings.append(self.m[off+2:off+2+ln].decode('utf-8', 'replace'))

    def node(self, i):
        name, child, nchild, t = struct.unpack_from('<IIHH', self.m, self.noff + i*20)
        data = self.m[self.noff + i*20 + 12: self.noff + i*20 + 20]
        return name, child, nchild, t, data

    def resolve(self, path):
        idx = 0
        for part in path:
            name, child, nchild, t, _ = self.node(idx)
            found = None
            for c in range(child, child + nchild):
                cn = self.node(c)
                if self.strings[cn[0]] == part:
                    found = c
                    break
            if found is None:
                return None
            idx = found
        return idx

    def describe(self, i):
        name, child, nchild, t, data = self.node(i)
        tn = ['none', 'int', 'real', 'str', 'vec', 'bmp', 'audio'][t]
        extra = ''
        if t == 5:
            _, w, h = struct.unpack('<IHH', data)
            extra = ' %dx%d' % (w, h)
        elif t == 4:
            x, y = struct.unpack('<ii', data)
            extra = ' (%d,%d)' % (x, y)
        elif t == 1:
            extra = ' %d' % struct.unpack('<q', data)[0]
        elif t == 3:
            extra = ' "%s"' % self.strings[struct.unpack('<I', data[:4])[0]]
        return '%s [%s%s] children=%d' % (self.strings[name], tn, extra, nchild)

    def ls(self, path, depth=1, indent=0, max_children=200):
        idx = self.resolve(path) if path else 0
        if idx is None:
            print('  ' * indent + '/'.join(path) + '  -- NOT FOUND')
            return
        name, child, nchild, t, _ = self.node(idx)
        if indent == 0:
            print('/' + '/'.join(path) + '  (%d children)' % nchild)
        for c in range(child, min(child + nchild, child + max_children)):
            print('  ' * (indent + 1) + self.describe(c))
            if depth > 1:
                cn = self.node(c)
                if cn[2] > 0:
                    self.ls_idx(c, depth - 1, indent + 1, max_children)
        if nchild > max_children:
            print('  ' * (indent + 1) + '... (%d more)' % (nchild - max_children))

    def ls_idx(self, idx, depth, indent, max_children=200):
        name, child, nchild, t, _ = self.node(idx)
        for c in range(child, min(child + nchild, child + max_children)):
            print('  ' * (indent + 1) + self.describe(c))
            if depth > 1:
                cn = self.node(c)
                if cn[2] > 0:
                    self.ls_idx(c, depth - 1, indent + 1, max_children)

if __name__ == '__main__':
    nx = NX(sys.argv[1])
    path = sys.argv[2].split('/') if len(sys.argv) > 2 and sys.argv[2] else []
    depth = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    nx.ls(path, depth)
