#include <zephyr/kernel.h>
#include "util.h"
LV_IMG_DECLARE(bolt);

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]) {
    static lv_color_t cbuf_tmp[CANVAS_SIZE * CANVAS_SIZE];
    memcpy(cbuf_tmp, cbuf, sizeof(cbuf_tmp));
    lv_img_dsc_t img;
    img.data = (void *)cbuf_tmp;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;
    img.header.w = CANVAS_SIZE;
    img.header.h = CANVAS_SIZE;
    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    lv_canvas_transform(canvas, &img, 900, LV_IMG_ZOOM_NONE, -1, 0, CANVAS_SIZE / 2, CANVAS_SIZE / 2, true);
}

void draw_battery(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_rect_dsc_t bg, fg;
    init_rect_dsc(&bg, LVGL_BACKGROUND);
    init_rect_dsc(&fg, LVGL_FOREGROUND);
    lv_canvas_draw_rect(canvas, 0, 2, 29, 12, &fg);
    lv_canvas_draw_rect(canvas, 1, 3, 27, 10, &bg);
    lv_canvas_draw_rect(canvas, 2, 4, (state->battery + 2) / 4, 8, &fg);
    lv_canvas_draw_rect(canvas, 30, 5, 3, 6, &fg);
    lv_canvas_draw_rect(canvas, 31, 6, 1, 4, &bg);
    if (state->charging) {
        lv_draw_img_dsc_t dsc;
        lv_draw_img_dsc_init(&dsc);
        lv_canvas_draw_img(canvas, 9, -1, &bolt, &dsc);
    }
}

void init_label_dsc(lv_draw_label_dsc_t *dsc, lv_color_t color, const lv_font_t *font, lv_text_align_t align) {
    lv_draw_label_dsc_init(dsc); dsc->color = color; dsc->font = font; dsc->align = align;
}
void init_rect_dsc(lv_draw_rect_dsc_t *dsc, lv_color_t color) {
    lv_draw_rect_dsc_init(dsc); dsc->bg_color = color;
}
