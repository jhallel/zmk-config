#include <lvgl.h>
#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t bolt_map[] = {
#if CONFIG_NICE_VIEW_WIDGET_INVERTED
    0xff,0xff,0xff,0xff, 0x00,0x00,0x00,0xff,
#else
    0x00,0x00,0x00,0xff, 0xff,0xff,0xff,0xff,
#endif
    0x10,0x30,0x60,0xe0,0x70,0x30,0x10,0x00,
};
const lv_img_dsc_t bolt = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 4,
    .header.h = 8,
    .data_size = 16,
    .data = bolt_map,
};
