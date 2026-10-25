# Draws the app icons and store graphics in the style of the PebbleOS icons
# (as in the Weather app): a flat fill with a thick black outline, chamfered
# corners and round line caps.
#   store/icon-144.png, store/icon-48.png, store/banner-720x320.png
#   resources/memosicon.png (25x25 menu icon)
# Usage: python3 store/make_art.py   (needs Pillow and the Inter font)
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
STORE = os.path.join(ROOT, 'store')
RESOURCES = os.path.join(ROOT, 'resources')

BLACK = (0, 0, 0, 255)
ORANGE = (255, 85, 0, 255)  # GColorOrange, the Pebble orange
WHITE = (255, 255, 255, 255)

# A shard of pottery with writing scratched onto it and a broken corner,
# in a 25x25 grid. src/c/ui.c draws the same shard, twice the
# size, for the loading animation.
SHARD = [(1.5, 7.5), (5.5, 3.5), (14.5, 3.5), (16.5, 6), (19, 5), (19.5, 7.5),
         (23, 9), (23, 15), (18, 20.5), (6.5, 20.5), (1.5, 15.5)]
WRITING = [[(5.5, 8), (14, 8)], [(5.5, 11.5), (9, 11.5)], [(11, 11.5), (19, 11.5)],
           [(5.5, 15), (12, 15)], [(14, 15), (18, 15)]]


def draw_shard(size, fill, outline, ink, ss=8):
    """Returns the shard at size x size, drawn ss times larger and scaled
    down for smooth edges. Widths are in units of the 25x25 grid."""
    big = size * ss
    k = big / 25.0
    ink_layer = Image.new('L', (big, big), 0)
    body = Image.new('L', (big, big), 0)
    di = ImageDraw.Draw(ink_layer)
    db = ImageDraw.Draw(body)

    def scaled(points):
        return [(x * k, y * k) for x, y in points]

    def stroke(points, width, closed=False):
        points = scaled(points)
        if closed:
            points = points + [points[0]]
        di.line(points, fill=255, width=max(1, round(width * k)))
        r = width * k / 2
        for x, y in points:  # round caps and joins
            di.ellipse([x - r, y - r, x + r, y + r], fill=255)

    db.polygon(scaled(SHARD), fill=255)
    stroke(SHARD, outline, closed=True)
    for line in WRITING:
        stroke(line, ink)

    ink_layer = ink_layer.resize((size, size), Image.LANCZOS)
    body = body.resize((size, size), Image.LANCZOS)
    img = Image.new('RGBA', (size, size), fill[:3] + (0,))
    img.putalpha(body)
    black = Image.new('RGBA', (size, size), BLACK)
    black.putalpha(ink_layer)
    img.alpha_composite(black)
    return img


# The 25x25 menu icon, drawn pixel by pixel as black line art like the
# icons of the PebbleOS launcher, which shows icons without their colors:
# the outline of the shard and its writing, see-through inside so it also
# looks right on the highlight
MENU_ICON = """
.........................
.........................
.........................
.....#########...........
....##########...##......
...##.......##..####.....
..##.........##.##.##....
.##...........###...##...
##...#######...#.....##..
##...#######..........##.
##....................##.
##....................##.
##...####.#########...##.
##...####.#########...##.
##....................##.
##...................##..
.##..#########......##...
..##.#########.....##....
...##.............##.....
....###############......
.....#############.......
.........................
.........................
.........................
.........................
"""


def menu_icon():
    img = Image.new('RGBA', (25, 25), (0, 0, 0, 0))
    for y, row in enumerate(MENU_ICON.strip('\n').split('\n')):
        for x, pixel in enumerate(row):
            if pixel == '#':
                img.putpixel((x, y), BLACK)
    return img


menu_icon().save(os.path.join(RESOURCES, 'memosicon.png'))

for size in (144, 48):
    pad = size // 16
    canvas = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    inner = size - 2 * pad
    # thinner lines in the small icon, like the 25px PebbleOS icons
    outline, ink = (1.5, 1.1) if size >= 100 else (1.8, 1.3)
    canvas.alpha_composite(draw_shard(inner, ORANGE, outline, ink),
                           (pad, pad))
    canvas.save(os.path.join(STORE, f'icon-{size}.png'))

# banner: the shard on the left, name and tagline on the right
W, H = 720, 320
banner = Image.new('RGBA', (W, H), WHITE)
d = ImageDraw.Draw(banner)
banner.alpha_composite(draw_shard(240, ORANGE, 1.4, 1.0), (45, 40))
title = ImageFont.truetype('/usr/share/fonts/opentype/inter/Inter-Bold.otf', 72)
sub = ImageFont.truetype('/usr/share/fonts/opentype/inter/InterDisplay-Medium.otf', 25)
d.text((320, 95), 'Ostrakon', font=title, fill=BLACK)
d.text((324, 190), 'Notes on your wrist · for Memos', font=sub, fill=(85, 85, 85))
# an orange underline, like a Pebble highlight
d.rounded_rectangle([324, 238, 384, 244], radius=3, fill=ORANGE)
banner.convert('RGB').save(os.path.join(STORE, 'banner-720x320.png'))
