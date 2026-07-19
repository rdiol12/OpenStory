"""Generate icon.ico for the GenMs launcher — a maple leaf on a rounded
parchment tile, matching the launcher's palette. Multi-size for crisp display."""
from PIL import Image, ImageDraw, ImageFont

SZ = 256
PARCH_TOP = (245, 231, 200, 255)   # warm parchment
PARCH_BOT = (233, 210, 160, 255)
WOOD      = (176, 122, 60, 255)    # maple wood border

img = Image.new("RGBA", (SZ, SZ), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

# vertical parchment gradient inside a rounded square
grad = Image.new("RGBA", (SZ, SZ))
gd = ImageDraw.Draw(grad)
for y in range(SZ):
    t = y / (SZ - 1)
    c = tuple(int(PARCH_TOP[i] + (PARCH_BOT[i] - PARCH_TOP[i]) * t) for i in range(4))
    gd.line([(0, y), (SZ, y)], fill=c)

mask = Image.new("L", (SZ, SZ), 0)
ImageDraw.Draw(mask).rounded_rectangle([6, 6, SZ - 6, SZ - 6], radius=54, fill=255)
img.paste(grad, (0, 0), mask)
d.rounded_rectangle([6, 6, SZ - 6, SZ - 6], radius=54, outline=WOOD, width=10)

# maple leaf — use the color emoji glyph if the system font is available
leaf_drawn = False
for fp in (r"C:\Windows\Fonts\seguiemj.ttf",):
    try:
        f = ImageFont.truetype(fp, 150)
        tmp = Image.new("RGBA", (200, 200), (0, 0, 0, 0))
        ImageDraw.Draw(tmp).text((100, 100), "\U0001F341", font=f,
                                 anchor="mm", embedded_color=True)
        img.alpha_composite(tmp, (SZ // 2 - 100, SZ // 2 - 96))
        leaf_drawn = True
        break
    except Exception as e:
        print("emoji leaf failed:", e)

if not leaf_drawn:
    # fallback: a simple stylised red maple leaf polygon
    cx, cy, s = SZ // 2, SZ // 2 + 6, 78
    pts = [(0, -1.0), (0.28, -0.42), (0.62, -0.55), (0.5, -0.16),
           (0.98, -0.02), (0.6, 0.2), (0.78, 0.62), (0.32, 0.42),
           (0.36, 0.9), (0, 0.6), (-0.36, 0.9), (-0.32, 0.42),
           (-0.78, 0.62), (-0.6, 0.2), (-0.98, -0.02), (-0.5, -0.16),
           (-0.62, -0.55), (-0.28, -0.42)]
    d.polygon([(cx + x * s, cy + y * s) for x, y in pts], fill=(206, 75, 58, 255))
    d.line([(cx, cy + 0.6 * s), (cx, cy - 0.2 * s)], fill=(150, 45, 35, 255), width=6)

img.save("icon.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print("wrote icon.ico")
