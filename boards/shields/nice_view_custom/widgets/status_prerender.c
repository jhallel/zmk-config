#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/wpm.h>

#include "status.h"

LV_IMG_DECLARE(nerv_bg_1);
LV_IMG_DECLARE(nerv_bg_2);
LV_IMG_DECLARE(nerv_bg_3);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static uint8_t wpm_history[10];

struct output_status_state {
    bool usb_selected;
    uint8_t profile_index;
    bool connected;
    bool bonded;
};
struct layer_status_state { uint8_t index; };
struct wpm_status_state { uint8_t wpm; };

static const char *mode_name(uint8_t layer) {
    switch (layer) {
    case 0: return "NORMAL";
    case 1: return "M1";
    case 2: return "M2";
    case 3: return "MAINT";
    default: return "SPEC";
    }
}

static void solid(lv_obj_t *canvas, int x, int y, int w, int h, lv_color_t color) {
    lv_draw_rect_dsc_t d;
    init_rect_dsc(&d, color);
    lv_canvas_draw_rect(canvas, x, y, w, h, &d);
}

static void line(lv_obj_t *canvas, int x1, int y1, int x2, int y2, int width) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = LVGL_FOREGROUND;
    d.width = width;
    lv_point_t pts[2] = {{x1, y1}, {x2, y2}};
    lv_canvas_draw_line(canvas, pts, 2, &d);
}

static uint16_t glyph3x5(char c) {
    switch (c) {
    case ' ': return 0x0000;
    case 'A': return 0x2bed; case 'B': return 0x6bae; case 'C': return 0x3923;
    case 'D': return 0x6b6e; case 'E': return 0x79a7; case 'F': return 0x79a4;
    case 'G': return 0x396b; case 'H': return 0x5bed; case 'I': return 0x7497;
    case 'J': return 0x126a; case 'K': return 0x5bad; case 'L': return 0x4927;
    case 'M': return 0x5fed; case 'N': return 0x5ffd; case 'O': return 0x2b6a;
    case 'P': return 0x6ba4; case 'Q': return 0x2b7b; case 'R': return 0x6bad;
    case 'S': return 0x388e; case 'T': return 0x7492; case 'U': return 0x5b6f;
    case 'V': return 0x5b6a; case 'W': return 0x5bfd; case 'X': return 0x5aad;
    case 'Y': return 0x5a92; case 'Z': return 0x72a7;
    case '0': return 0x7b6f; case '1': return 0x2c97; case '2': return 0x62a7;
    case '3': return 0x628e; case '4': return 0x5bc9; case '5': return 0x798e;
    case '6': return 0x39aa; case '7': return 0x7292; case '8': return 0x2aaa;
    case '9': return 0x2ace; case '%': return 0x52a5; case '-': return 0x01c0;
    default: return 0;
    }
}

static void pixel_text(lv_obj_t *canvas, int x, int y, const char *s, bool inverse) {
    lv_color_t color = inverse ? LVGL_BACKGROUND : LVGL_FOREGROUND;
    for (int i = 0; s[i] != '\0'; i++) {
        uint16_t bits = glyph3x5(s[i]);
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                int bit = 14 - (row * 3 + col);
                if ((bits >> bit) & 1U) {
                    solid(canvas, x + col, y + row, 1, 1, color);
                }
            }
        }
        x += 4;
    }
}

static void draw_bg(lv_obj_t *canvas, const lv_img_dsc_t *bg) {
    lv_draw_rect_dsc_t clear;
    init_rect_dsc(&clear, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &clear);
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, 0, 0, bg, &img_dsc);
}

static void profile_fill(lv_obj_t *canvas, int cx, int cy, int n) {
    solid(canvas, cx - 4, cy - 4, 9, 9, LVGL_FOREGROUND);
    char label[2] = {(char)('0' + n), '\0'};
    pixel_text(canvas, cx - 2, cy - 2, label, true);
}

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    draw_bg(canvas, &nerv_bg_1);

    int fill_w = (state->battery * 34) / 100;
    if (fill_w > 0) {
        solid(canvas, 4, 36, fill_w, 8, LVGL_FOREGROUND);
    }

    char pct[8];
    snprintf(pct, sizeof(pct), "%u%%", state->battery);
    pixel_text(canvas, 52, 37, pct, false);

    const char *link = state->usb_selected ? "USB" : (state->connected ? "ONLINE" : "STBY");
    pixel_text(canvas, 14, 62, link, false);
    rotate_canvas(canvas, cbuf);
}

static void draw_middle(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);
    draw_bg(canvas, &nerv_bg_2);

    int max = 1;
    for (int i = 0; i < 10; i++) {
        if (wpm_history[i] > max) max = wpm_history[i];
    }
    for (int i = 0; i < 10; i++) {
        int h = 2 + (wpm_history[i] * 17) / max;
        line(canvas, 3 + i * 4, 26, 3 + i * 4, 26 - h, 2);
    }

    char wpm[8];
    snprintf(wpm, sizeof(wpm), "%03u", state->wpm);
    pixel_text(canvas, 46, 11, wpm, false);
    pixel_text(canvas, 46, 19, "WPM", false);

    if (!state->usb_selected && state->profile_index < 5) {
        static const int centers[5] = {7, 20, 33, 46, 59};
        profile_fill(canvas, centers[state->profile_index], 49, state->profile_index + 1);
    }

    pixel_text(canvas, 40, 61,
               state->usb_selected ? "USB" : (state->bonded ? "ON" : "WAIT"), false);
    rotate_canvas(canvas, cbuf);
}

static void draw_bottom(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);
    draw_bg(canvas, &nerv_bg_3);
    pixel_text(canvas, 34, 15, mode_name(state->layer_index), false);
    rotate_canvas(canvas, cbuf);
}

static void redraw_all(struct zmk_widget_status *widget) {
    draw_top(widget->obj, widget->cbuf, &widget->state);
    draw_middle(widget->obj, widget->cbuf2, &widget->state);
    draw_bottom(widget->obj, widget->cbuf3, &widget->state);
}

static void set_battery_status(struct zmk_widget_status *widget, struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif
    widget->state.battery = state.level;
    draw_top(widget->obj, widget->cbuf, &widget->state);
}
static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}
static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif

static void set_output_status(struct zmk_widget_status *widget,
                              const struct output_status_state *state) {
    widget->state.usb_selected = state->usb_selected;
    widget->state.profile_index = state->profile_index;
    widget->state.connected = state->connected;
    widget->state.bonded = state->bonded;
    draw_top(widget->obj, widget->cbuf, &widget->state);
    draw_middle(widget->obj, widget->cbuf2, &widget->state);
}
static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}
static struct output_status_state output_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    struct zmk_endpoint_instance ep = zmk_endpoints_selected();
    return (struct output_status_state){
        .usb_selected = ep.transport == ZMK_TRANSPORT_USB,
        .profile_index = zmk_ble_active_profile_index(),
        .connected = zmk_ble_active_profile_is_connected(),
        .bonded = !zmk_ble_active_profile_is_open(),
    };
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    draw_bottom(widget->obj, widget->cbuf3, &widget->state);
}
static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}
static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct layer_status_state){.index = zmk_keymap_highest_layer_active()};
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state,
                            layer_status_update_cb, layer_status_get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    for (int i = 0; i < 9; i++) wpm_history[i] = wpm_history[i + 1];
    wpm_history[9] = state.wpm;
    widget->state.wpm = state.wpm;
    draw_middle(widget->obj, widget->cbuf2, &widget->state);
}
static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}
static struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state,
                            wpm_status_update_cb, wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *middle = lv_canvas_create(widget->obj);
    lv_obj_align(middle, LV_ALIGN_TOP_LEFT, 24, 0);
    lv_canvas_set_buffer(middle, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, -44, 0);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    widget->state.battery = zmk_battery_state_of_charge();
    widget->state.wpm = zmk_wpm_get_state();
    for (int i = 0; i < 10; i++) wpm_history[i] = widget->state.wpm;
    widget->state.layer_index = zmk_keymap_highest_layer_active();

    struct zmk_endpoint_instance ep = zmk_endpoints_selected();
    widget->state.usb_selected = ep.transport == ZMK_TRANSPORT_USB;
    widget->state.profile_index = zmk_ble_active_profile_index();
    widget->state.connected = zmk_ble_active_profile_is_connected();
    widget->state.bonded = !zmk_ble_active_profile_is_open();

    sys_slist_append(&widgets, &widget->node);
    redraw_all(widget);
    widget_battery_status_init();
    widget_output_status_init();
    widget_layer_status_init();
    widget_wpm_status_init();
    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
