#pragma once
#include <lvgl.h>
#define CANVAS_SIZE 68

/* The central/left display is intentionally inverted for the NERV terminal:
 * black background with white UI. The peripheral/right display keeps the
 * existing palette rules so the EVA animation remains unchanged. */
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#define LVGL_BACKGROUND lv_color_black()
#define LVGL_FOREGROUND lv_color_white()
#else
#define LVGL_BACKGROUND (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black() : lv_color_white())
#define LVGL_FOREGROUND (IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white() : lv_color_black())
#endif

struct status_state {
    uint8_t battery;
    bool charging;
    bool connected;
    bool bonded;
    bool usb_selected;
    uint8_t profile_index;
    uint8_t wpm;
    uint8_t layer_index;
};
struct battery_status_state {
    uint8_t level;
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    bool usb_present;
#endif
};

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]);
void draw_battery(lv_obj_t *canvas, const struct status_state *state);
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font, lv_text_align_t align);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
