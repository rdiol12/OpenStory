"""Rebuild UI.nx with the v83 WorldSelect art grafted in.

Reads the existing UI.nx, replaces Login.img/WorldSelect children with the
v83-extracted subtrees (keeping extra modern nodes like worldNotice/tooltip),
and writes a new PKG4 file. Kept bitmaps copy their compressed blocks
verbatim; new ones are LZ4-compressed from the extracted PNGs.
"""
import struct, json, os, sys
import lz4.block
from PIL import Image

SRC = r'C:\Users\rdiol\OpenStory\wz\UI.nx'
OUT = r'C:\Users\rdiol\OpenStory\wz\UI.nx.new'
V83 = 'v83ws'

data = open(SRC, 'rb').read()
magic, ncount, noff, scount, soff, bcount, boff, acount, aoff = struct.unpack_from('<4sIQIQIQIQ', data, 0)
assert magic == b'PKG4' and acount == 0

str_offsets = struct.unpack_from('<%dQ' % scount, data, soff)
def get_string(i):
    off = str_offsets[i]
    ln = struct.unpack_from('<H', data, off)[0]
    return data[off+2:off+2+ln]

bmp_offsets = struct.unpack_from('<%dQ' % bcount, data, boff)
def get_bmp_block(i):
    off = bmp_offsets[i]
    ln = struct.unpack_from('<I', data, off)[0]
    return data[off:off+4+ln]

# ---- load original tree ----
class N:
    __slots__ = ('name', 'type', 'data', 'children')
    def __init__(self, name, type_, data_, children=None):
        self.name = name          # bytes
        self.type = type_
        self.data = data_         # for type 5: ('oldbmp', id, w, h) or ('newbmp', bytes, w, h); type 3: ('str', bytes); else raw 8 bytes
        self.children = children or []

def load(idx):
    nameid, child, num, typ = struct.unpack_from('<IIHH', data, noff + idx*20)
    raw = data[noff + idx*20 + 12: noff + idx*20 + 20]
    n = N(get_string(nameid), typ, raw)
    if typ == 3:
        sid = struct.unpack_from('<I', raw, 0)[0]
        n.data = ('str', get_string(sid))
    elif typ == 5:
        bid, w, h = struct.unpack('<IHH', raw)
        n.data = ('oldbmp', bid, w, h)
    n.children = [load(c) for c in range(child, child + num)]
    return n

print('loading original tree...')
root = load(0)

def find(node, path):
    for part in path:
        for c in node.children:
            if c.name == part:
                node = c
                break
        else:
            return None
    return node

ws = find(root, [b'Login.img', b'WorldSelect'])
assert ws is not None, 'WorldSelect not found'

# ---- build v83 subtrees ----
def make_int(name, v):
    return N(name, 1, struct.pack('<q', int(v)))

def make_double(name, v):
    return N(name, 2, struct.pack('<d', float(v)))

def make_vec(name, x, y):
    return N(name, 4, struct.pack('<ii', int(x), int(y)))

def make_str(name, s):
    return N(name, 3, ('str', s.encode('utf-8')))

def build(name, val):
    if isinstance(val, dict):
        if '__canvas' in val:
            img = Image.open(os.path.join(V83, val['__canvas'])).convert('RGBA')
            w, h = img.size
            r, g, b, a = img.split()
            bgra = Image.merge('RGBA', (b, g, r, a)).tobytes()
            n = N(name, 5, ('newbmp', bgra, w, h))
        elif '__vector' in val:
            return make_vec(name, val['__vector'][0], val['__vector'][1])
        elif '__uol' in val or '__sound' in val:
            return None
        else:
            n = N(name, 0, b'\0' * 8)
        for k, v in val.items():
            if k.startswith('__'):
                continue
            c = build(k.encode('utf-8'), v)
            if c is not None:
                n.children.append(c)
        return n
    if isinstance(val, bool):
        return make_int(name, int(val))
    if isinstance(val, int):
        return make_int(name, val)
    if isinstance(val, float):
        return make_double(name, val)
    if isinstance(val, str):
        return make_str(name, val)
    if val is None:
        return N(name, 0, b'\0' * 8)
    raise ValueError('bad json value %r' % (val,))

tree = json.load(open(os.path.join(V83, 'tree.json')))
REPLACE = ['scroll', 'BtWorld', 'world', 'channel', 'chBackgrn', 'BtGoworld', 'BtViewAll', 'BtViewChoice', 'alert']
new_children = {}
for key in REPLACE:
    if key in tree:
        new_children[key.encode('utf-8')] = build(key.encode('utf-8'), tree[key])

kept = [c for c in ws.children if c.name not in new_children]
ws.children = kept + list(new_children.values())
print('WorldSelect children now:', sorted(c.name.decode() for c in ws.children))

# ---- serialize ----
print('serializing...')
strings = {}
def sid(s):
    if s not in strings:
        strings[s] = len(strings)
    return strings[s]

bitmaps = []   # list of ('old', id) | ('new', compressed_bytes)
old_bmp_map = {}  # original bitmap id -> new table index (nodes share bitmaps)
def bid_for(node):
    if node.data[0] == 'oldbmp':
        oid = node.data[1]
        if oid not in old_bmp_map:
            old_bmp_map[oid] = len(bitmaps)
            bitmaps.append(('old', oid))
        return old_bmp_map[oid], node.data[2], node.data[3]
    i = len(bitmaps)
    raw = node.data[1]
    comp = lz4.block.compress(raw, store_size=False)
    bitmaps.append(('new', struct.pack('<I', len(comp)) + comp))
    return i, node.data[2], node.data[3]

# BFS with contiguous, name-sorted children
order = [root]
first_child = {}
i = 0
while i < len(order):
    n = order[i]
    n.children.sort(key=lambda c: c.name)
    first_child[id(n)] = len(order)
    order.extend(n.children)
    i += 1

print('total nodes:', len(order))
node_recs = []
for n in order:
    nameid = sid(n.name)
    child = first_child[id(n)]
    num = len(n.children)
    if n.type == 3:
        raw = struct.pack('<II', sid(n.data[1]), 0)
    elif n.type == 5:
        b, w, h = bid_for(n)
        raw = struct.pack('<IHH', b, w, h)
    else:
        raw = n.data if isinstance(n.data, bytes) else b'\0' * 8
    node_recs.append(struct.pack('<IIHH', nameid, child, num, n.type) + raw)

def align8(buf):
    while len(buf) % 8:
        buf.append(0)

out = bytearray(52)
align8(out)
new_noff = len(out)
for r in node_recs:
    out += r
align8(out)

# string data + table
str_list = sorted(strings.items(), key=lambda kv: kv[1])
str_positions = []
for s, _ in str_list:
    str_positions.append(len(out))
    out += struct.pack('<H', len(s)) + s
align8(out)
new_soff = len(out)
for p in str_positions:
    out += struct.pack('<Q', p)
align8(out)

# bitmap data + table
bmp_positions = []
for kind, payload in bitmaps:
    align8(out)
    bmp_positions.append(len(out))
    if kind == 'old':
        out += get_bmp_block(payload)
    else:
        out += payload
align8(out)
new_boff = len(out)
for p in bmp_positions:
    out += struct.pack('<Q', p)

struct.pack_into('<4sIQIQIQIQ', out, 0, b'PKG4',
                 len(node_recs), new_noff,
                 len(str_list), new_soff,
                 len(bitmaps), new_boff,
                 0, 0)

open(OUT, 'wb').write(out)
print('wrote', OUT, len(out) // (1024*1024), 'MB, bitmaps:', len(bitmaps))
