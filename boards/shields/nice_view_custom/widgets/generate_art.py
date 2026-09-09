#!/usr/bin/env python3
"""Generate 32 full-height LVGL 1-bit Evangelion frames for nice!view."""
from pathlib import Path
import base64
import sys
import zlib

SRC_WIDTH = 140
HEIGHT = 68
SRC_ROW_BYTES = 18
SRC_FRAME_BYTES = SRC_ROW_BYTES * HEIGHT
OUT_WIDTH = 160
OUT_ROW_BYTES = 20
OUT_FRAME_BYTES = OUT_ROW_BYTES * HEIGHT
FRAME_COUNT = 32

PAYLOAD = "".join(
    (Path(__file__).with_name(f"eva_payload_{i}.txt").read_text().strip())
    for i in range(1, 7)
)


def get_pixel(raw, frame_idx, x, y):
    offset = frame_idx * SRC_FRAME_BYTES + y * SRC_ROW_BYTES + (x // 8)
    mask = 1 << (7 - (x % 8))
    return 1 if raw[offset] & mask else 0


def set_pixel(raw, frame_idx, x, y, value):
    offset = frame_idx * SRC_FRAME_BYTES + y * SRC_ROW_BYTES + (x // 8)
    mask = 1 << (7 - (x % 8))
    if value:
        raw[offset] |= mask
    else:
        raw[offset] &= ~mask


def strip_source_frame_labels(raw):
    """Remove the 25..32 sheet-label bleed from eva17..eva24 only."""
    for frame_idx in range(16, 24):
        for y in range(0, 18):
            for x in range(0, 10):
                set_pixel(raw, frame_idx, x, y, 0)


def stretch_frame(raw, frame_idx):
    """Expand 140x68 to 160x68 so the mounted portrait view fills 68x160."""
    out = bytearray(OUT_FRAME_BYTES)
    for y in range(HEIGHT):
        for x in range(OUT_WIDTH):
            src_x = (x * SRC_WIDTH) // OUT_WIDTH
            if get_pixel(raw, frame_idx, src_x, y):
                off = y * OUT_ROW_BYTES + (x // 8)
                out[off] |= 1 << (7 - (x % 8))
    return out


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_art.py OUTPUT_C")
    out_path = Path(sys.argv[1])
    raw = bytearray(zlib.decompress(base64.b85decode(PAYLOAD.encode("ascii"))))
    expected = FRAME_COUNT * SRC_FRAME_BYTES
    if len(raw) != expected:
        raise SystemExit(f"bad payload size: {len(raw)} != {expected}")

    strip_source_frame_labels(raw)

    lines = [
        "#include <lvgl.h>",
        "",
        "#ifndef LV_ATTRIBUTE_MEM_ALIGN",
        "#define LV_ATTRIBUTE_MEM_ALIGN",
        "#endif",
        "",
    ]
    for idx in range(FRAME_COUNT):
        name = f"eva{idx + 1:02d}"
        data = stretch_frame(raw, idx)
        lines += [
            f"const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t {name}_map[] = {{",
            "#if CONFIG_NICE_VIEW_WIDGET_INVERTED",
            "    0xff, 0xff, 0xff, 0xff,",
            "    0x00, 0x00, 0x00, 0xff,",
            "#else",
            "    0x00, 0x00, 0x00, 0xff,",
            "    0xff, 0xff, 0xff, 0xff,",
            "#endif",
        ]
        for off in range(0, len(data), OUT_ROW_BYTES):
            chunk = data[off:off + OUT_ROW_BYTES]
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
        lines += [
            "};",
            "",
            f"const lv_img_dsc_t {name} = {{",
            "    .header.cf = LV_IMG_CF_INDEXED_1BIT,",
            "    .header.always_zero = 0,",
            "    .header.reserved = 0,",
            f"    .header.w = {OUT_WIDTH},",
            f"    .header.h = {HEIGHT},",
            f"    .data_size = {OUT_FRAME_BYTES + 8},",
            f"    .data = {name}_map,",
            "};",
            "",
        ]
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines))


if __name__ == "__main__":
    main()
