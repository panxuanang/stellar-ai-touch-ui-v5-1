#!/usr/bin/env python3
"""Convert a PNG/JPG into an LVGL 9 RGB565 C image descriptor.

Example for Touch UI V4:
  python scripts/image_to_lvgl_rgb565.py \
      preview/stellar_avatar_200x320.png \
      overlay/main/display/stellar/stellar_avatar.c \
      stellar_avatar 200 320

Pillow is required only when replacing the picture. GitHub Actions does not run
this helper because the generated C asset is already committed in the project.
"""
from pathlib import Path
import sys
from PIL import Image

if len(sys.argv) != 6:
    raise SystemExit("Usage: image_to_lvgl_rgb565.py INPUT OUTPUT_C SYMBOL WIDTH HEIGHT")

source, output, symbol, width, height = (
    sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
)
img = Image.open(source).convert("RGB").resize((width, height), Image.Resampling.LANCZOS)
raw = []
for r, g, b in img.getdata():
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    raw.extend((value & 0xFF, value >> 8))

macro = "LV_ATTRIBUTE_" + symbol.upper()
with Path(output).open("w", encoding="utf-8") as f:
    f.write('#include <lvgl.h>\n#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n')
    f.write(f'#ifndef {macro}\n#define {macro}\n#endif\n')
    f.write(
        f'static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST {macro} '
        f'uint8_t {symbol}_map[] = {{\n'
    )
    for i in range(0, len(raw), 16):
        f.write('    ' + ','.join(f'0x{x:02x}' for x in raw[i:i+16]) + ',\n')
    f.write('};\n')
    f.write(f'const lv_image_dsc_t {symbol} = {{\n')
    f.write('    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565, .flags = 0,')
    f.write(f' .w = {width}, .h = {height}, .stride = {width * 2}, .reserved_2 = 0 }},\n')
    f.write(f'    .data_size = sizeof({symbol}_map), .data = {symbol}_map, .reserved = NULL,\n}};\n')
print(f"wrote {output}")
