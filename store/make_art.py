# Draws the app store graphics: icon-144.png, icon-48.png, banner-720x320.png
# Usage: python3 store/make_art.py store   (needs Pillow, DejaVu and Inter fonts)
import sys, random
from PIL import Image, ImageDraw, ImageFilter, ImageFont

OUT = sys.argv[1]
CLAY = (190, 98, 58)        # terracotta
CLAY_LIGHT = (214, 128, 84)
CLAY_DARK = (128, 58, 30)
INK = (70, 32, 16)

# the same shard outline as the watch icon, in a 25x25 grid
SHARD = [(0.5, 7), (4.5, 3.5), (8, 4.2), (12.5, 1.5), (17.5, 3), (24.5, 5),
         (23, 9.5), (24.5, 13), (21, 17.5), (22, 20.5), (16, 23.5),
         (13, 21), (8.5, 22.5), (6, 18.5), (2, 17), (3, 12)]
WRITING = [(8, [(4, 9), (12, 20)]), (12, [(5, 11), (14, 20)]), (16, [(7, 14)])]

def shard_image(size):
    """Renders the shard at 4x and scales it down for smooth edges."""
    big = size * 4
    k = big / 25.0
    img = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    pts = [(x * k, y * k) for x, y in SHARD]
    # soft shadow
    shadow = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    ImageDraw.Draw(shadow).polygon([(x + k * 0.5, y + k * 0.7) for x, y in pts], fill=(60, 30, 15, 90))
    img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(k * 0.5)))
    # clay body with a darker broken edge
    d.polygon(pts, fill=CLAY_DARK)
    cx = sum(x for x, _ in pts) / len(pts)
    cy = sum(y for _, y in pts) / len(pts)
    inner = [(cx + (x - cx) * 0.93, cy + (y - cy) * 0.93) for x, y in pts]
    d.polygon(inner, fill=CLAY)
    # lighter glaze toward the top left, like light on a curved pot
    glaze = Image.new('L', (big, big), 0)
    ImageDraw.Draw(glaze).ellipse([-big * 0.3, -big * 0.35, big * 0.75, big * 0.6], fill=110)
    glaze = glaze.filter(ImageFilter.GaussianBlur(k * 2))
    mask = Image.new('L', (big, big), 0)
    ImageDraw.Draw(mask).polygon(inner, fill=255)
    light = Image.new('RGBA', (big, big), CLAY_LIGHT + (0,))
    light.putalpha(Image.composite(glaze, Image.new('L', (big, big), 0), mask))
    img.alpha_composite(light)
    # scratched writing: made-up glyphs of short strokes, hooks and arcs
    # that look hand-written but belong to no real script
    text = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    td = ImageDraw.Draw(text)
    rnd = random.Random(11)
    stroke = max(2, int(k * 0.42))
    glyph_w, glyph_h = 1.55 * k, 2.3 * k

    def glyph(x, y):
        for _ in range(rnd.randint(2, 3)):
            kind = rnd.random()
            if kind < 0.35:    # slanted stroke
                x0 = x + rnd.uniform(0, glyph_w * 0.6)
                td.line([(x0, y + glyph_h), (x0 + rnd.uniform(-0.4, 0.6) * glyph_w, y)],
                        fill=INK, width=stroke)
            elif kind < 0.6:   # hook: down and a short tail
                x0 = x + rnd.uniform(0, glyph_w * 0.5)
                mid = y + rnd.uniform(0.5, 1.0) * glyph_h
                td.line([(x0, y), (x0, mid), (x0 + glyph_w * 0.5, mid - rnd.uniform(0, 0.4) * glyph_h)],
                        fill=INK, width=stroke, joint='curve')
            elif kind < 0.85:  # small arc
                box = [x, y + rnd.uniform(0, 0.4) * glyph_h, x + glyph_w, y + glyph_h]
                start_angle = rnd.choice([0, 90, 180, 270])
                td.arc(box, start_angle, start_angle + rnd.randint(120, 220), fill=INK, width=stroke)
            else:              # short crossbar
                yy = y + rnd.uniform(0.2, 0.8) * glyph_h
                td.line([(x, yy), (x + glyph_w, yy + rnd.uniform(-0.2, 0.2) * glyph_h)],
                        fill=INK, width=stroke)

    # lines of "words", shorter where the shard narrows
    for line_y, words in [(5.6, [(4.2, 4), (12.6, 4)]), (10.0, [(5.0, 3), (11.4, 4)]),
                          (14.4, [(7.4, 4)])]:
        for x, glyphs in words:
            for i in range(glyphs):
                glyph((x + i * 1.85) * k, line_y * k)
    text = text.rotate(-4, resample=Image.BICUBIC, center=(big / 2, big / 2))
    # keep the writing inside the shard
    text.putalpha(Image.composite(text.getchannel('A'), Image.new('L', (big, big), 0), mask))
    img.alpha_composite(text)
    return img.resize((size, size), Image.LANCZOS)

for size in (144, 48):
    pad = size // 12
    canvas = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    canvas.alpha_composite(shard_image(size - 2 * pad), (pad, pad))
    canvas.save(f'{OUT}/icon-{size}.png')

# banner: shard on the left, name and tagline on the right
W, H = 720, 320
banner = Image.new('RGBA', (W, H), (245, 238, 228, 255))
d = ImageDraw.Draw(banner)
banner.alpha_composite(shard_image(230), (50, 45))
title = ImageFont.truetype('/usr/share/fonts/opentype/inter/Inter-Bold.otf', 72)
sub = ImageFont.truetype('/usr/share/fonts/opentype/inter/InterDisplay-Medium.otf', 25)
d.text((320, 95), 'Ostrakon', font=title, fill=INK)
d.text((324, 190), 'Notes on your wrist · for Memos', font=sub, fill=CLAY_DARK)
banner.convert('RGB').save(f'{OUT}/banner-720x320.png')
