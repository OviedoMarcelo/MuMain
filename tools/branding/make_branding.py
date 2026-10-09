"""Builds the MU Cyber logo and loading screens in the client's OZT/OZJ formats.

usage: make_branding.py <logo.png> <banner image> <client Data dir> <preview dir>
"""
import io
import struct
import sys
from pathlib import Path

from PIL import Image, ImageFilter

logo_path, banner_path, data_dir, preview_dir = (Path(a) for a in sys.argv[1:5])
preview_dir.mkdir(parents=True, exist_ok=True)


def write_ozj(img: Image.Image, path: Path):
    buf = io.BytesIO()
    img.convert("RGB").save(buf, "JPEG", quality=92, subsampling=0)
    jpeg = buf.getvalue()
    path.write_bytes(jpeg[:24] + jpeg)  # the client skips a 24-byte header


def write_ozt(img: Image.Image, path: Path):
    img = img.convert("RGBA")
    w, h = img.size
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 32, 8)
    pixels = bytearray()
    for y in range(h - 1, -1, -1):  # bottom-up, BGRA
        for x in range(w):
            r, g, b, a = img.getpixel((x, y))
            pixels += bytes((b, g, r, a))
    path.write_bytes(b"\0\0\x02\0" + header + pixels)


# --- Login screen logo: 512x256 texture, logo centered at full height --------------------------
logo = Image.open(logo_path).convert("RGBA")
logo_h = 256
logo_small = logo.resize((round(logo.width * logo_h / logo.height), logo_h), Image.LANCZOS)
canvas = Image.new("RGBA", (512, 256), (0, 0, 0, 0))
canvas.alpha_composite(logo_small, ((512 - logo_small.width) // 2, 0))
write_ozt(canvas, data_dir / "Logo" / "MU-logo.OZT")

# Glow drawn additively behind the logo: the logo on black, blurred and dimmed.
glow = Image.new("RGB", (512, 256))
glow.paste(canvas, mask=canvas.getchannel("A"))
glow = glow.filter(ImageFilter.GaussianBlur(8)).point(lambda v: int(v * 0.55))
write_ozj(glow, data_dir / "Logo" / "MU-logo_g.OZJ")
canvas.save(preview_dir / "logo-texture.png")

# --- Full-screen loading art, 16:9 ---------------------------------------------------------------
W, H = 1920, 1080
screen = Image.new("RGB", (W, H), (6, 4, 10))
banner = Image.open(banner_path).convert("RGB")
banner = banner.resize((W, round(banner.height * W / banner.width)), Image.LANCZOS)
top = 95
ramp = 70  # the banner's top and bottom edges fade into the dark frame
mask = Image.new("L", banner.size, 255)
for y in range(ramp):
    v = int(255 * y / ramp)
    mask.paste(v, (0, y, W, y + 1))
    mask.paste(v, (0, banner.height - 1 - y, W, banner.height - y))
screen.paste(banner, (0, top), mask)
small = logo.resize((round(logo.width * 300 / logo.height), 300), Image.LANCZOS)
screen.paste(small, (int(W * 0.27) - small.width // 2, 40), small)
screen.save(preview_dir / "loading-screen.png")


def tile(x0, y0, x1, y1, size):
    """The part of the screen between the fractions (x0, y0) and (x1, y1), at the texture size."""
    return screen.crop((round(x0 * W), round(y0 * H), round(x1 * W), round(y1 * H))).resize(size, Image.LANCZOS)


iface = data_dir / "Interface"

# Title (first loading) scene, see CUIMng::CreateTitleSceneUI: an 800x600 frame and a 1280x1024 middle.
write_ozj(tile(0, 0, 0.5, 69 / 600, (400, 69)), iface / "New_lo_back_01.OZJ")
write_ozj(tile(0.5, 0, 1, 69 / 600, (400, 69)), iface / "New_lo_back_02.OZJ")
write_ozj(tile(0, 500 / 600, 0.5, 1, (400, 100)), iface / "lo_back_s5_03.OZJ")
write_ozj(tile(0.5, 500 / 600, 1, 1, (400, 100)), iface / "lo_back_s5_04.OZJ")
middle = [  # (x, y, w, h) in 1280x1024
    (0, 119, 512, 512), (512, 119, 512, 512), (1024, 119, 256, 512),
    (0, 631, 512, 223), (512, 631, 512, 223), (1024, 631, 256, 223),
]
for n, (x, y, w, h) in enumerate(middle, start=1):
    img = tile(x / 1280, y / 1024, (x + w) / 1280, (y + h) / 1024, (w, h))
    for prefix in ("lo_back_im", "lo_back_s5_im"):  # both random themes show the same art
        write_ozj(img, iface / f"{prefix}0{n}.OZJ")

# Loading scene when entering the world, see CLoadingScene::Create: 800x600 in four parts.
write_ozj(tile(0, 0, 0.5, 512 / 600, (400, 512)), iface / "LSBg01.OZJ")
write_ozj(tile(0.5, 0, 1, 512 / 600, (400, 512)), iface / "LSBg02.OZJ")
write_ozj(tile(0, 512 / 600, 0.5, 1, (400, 88)), iface / "LSBg03.OZJ")
write_ozj(tile(0.5, 512 / 600, 1, 1, (400, 88)), iface / "LSBg04.OZJ")
print("ok")
