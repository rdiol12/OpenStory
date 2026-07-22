"""Extract the ENTIRE v83 Login.img/WorldSelect subtree: PNGs + structure JSON."""
import os, json
from wz import WzFile

wz = WzFile(r'C:\Nexon\MapleStory\UI.wz', (0x4D, 0x23, 0xC7, 0x2B))
img = wz.open_img(['Login.img'])
root, _ = img.parse()
ws = root['WorldSelect']

OUT = 'v83ws'
os.makedirs(OUT, exist_ok=True)
counter = [0]

def conv(node, path):
    """Convert to JSON-able structure; canvases saved as PNG files."""
    if isinstance(node, dict):
        t = node.get('__type')
        if t == 'canvas':
            fid = counter[0]; counter[0] += 1
            fn = 'bmp%05d.png' % fid
            img.decode_canvas(node).save(os.path.join(OUT, fn))
            out = {'__canvas': fn, '__w': node['__w'], '__h': node['__h']}
            for k, v in node.items():
                if not k.startswith('__'):
                    out[k] = conv(v, path + '/' + k)
            return out
        if t == 'vector':
            return {'__vector': [node['x'], node['y']]}
        if t == 'uol':
            return {'__uol': node['path']}
        if t == 'sound':
            return {'__sound': True}
        return {k: conv(v, path + '/' + k) for k, v in node.items() if not k.startswith('__')}
    return node

tree = conv(ws, 'WorldSelect')
json.dump(tree, open(os.path.join(OUT, 'tree.json'), 'w'), indent=1)
print('canvases:', counter[0])
print('top keys:', sorted(tree.keys()))
for k in ['world', 'channel', 'BtWorld', 'BtGoworld']:
    if k in tree:
        sub = tree[k]
        print(k, ':', sorted(sub.keys())[:24] if isinstance(sub, dict) else sub)
