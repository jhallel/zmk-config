#!/usr/bin/env python3
"""Generate three 68x68 1-bit NERV background tiles for the central nice!view."""
from pathlib import Path
import base64
import sys
import zlib

WIDTH = 68
HEIGHT = 68
ROW_BYTES = 9
TILE_BYTES = ROW_BYTES * HEIGHT
TILE_COUNT = 3

PAYLOAD = Path(__file__).with_name("nerv_bg_payload.txt").read_text().strip()


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_nerv_bg.py OUTPUT_C")
    out = Path(sys.argv[1])
    raw = zlib.decompress(base64.b85decode(PAYLOAD.encode("ascii")))
    expected = TILE_COUNT * TILE_BYTES
    if len(raw) != expected:
        raise SystemExit(f"bad NERV background payload size: {len(raw)} != {expected}")

    lines = [
        "#include <lvgl.h>",
        "",
        "#ifndef LV_ATTRIBUTE_MEM_ALIGN",
        "#define LV_ATTRIBUTE_MEM_ALIGN",
        "#endif",
        "",
    ]
    for idx in range(TILE_COUNT):
        name = f"nerv_bg_{idx + 1}"
        data = raw[idx * TILE_BYTES:(idx + 1) * TILE_BYTES]
        lines += [
            f"const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t {name}_map[] = {{",
            "    0x00, 0x00, 0x00, 0xff,",
            "    0xff, 0xff, 0xff, 0xff,",
        ]
        for off in range(0, len(data), 18):
            chunk = data[off:off + 18]
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
        lines += [
            "};",
            "",
            f"const lv_img_dsc_t {name} = {{",
            "    .header.cf = LV_IMG_CF_INDEXED_1BIT,",
            "    .header.always_zero = 0,",
            "    .header.reserved = 0,",
            f"    .header.w = {WIDTH},",
            f"    .header.h = {HEIGHT},",
            f"    .data_size = {TILE_BYTES + 8},",
            f"    .data = {name}_map,",
            "};",
            "",
        ]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines))


if __name__ == "__main__":
    main()
