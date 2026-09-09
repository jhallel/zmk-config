#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

#include "status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

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
    case 1: return "MAGI-01";
    case 2: return "MAGI-02";
    case 3: return "MAINT.";
    default: return "SPECIAL";
    }
}

static void fill(lv_obj_t *canvas, lv_color_t color) {
    lv_draw_rect_dsc_t d;
    init_rect_dsc(&d, color);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &d);
}

static void box(lv_obj_t *canvas, int x, int y, int w, int h, bool filled) {
    lv_draw_rect_dsc_t d;
    init_rect_dsc(&d, filled ? LVGL_FOREGROUND : LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, x, y, w, h, &d);
    if (!filled) {
        init_rect_dsc(&d, LVGL_FOREGROUND);
        lv_canvas_draw_rect(canvas, x, y, w, 1, &d);
        lv_canvas_draw_rect(canvas, x, y + h - 1, w, 1, &d);
        lv_canvas_draw_rect(canvas, x, y, 1, h, &d);
        lv_canvas_draw_rect(canvas, x + w - 1, y, 1, h, &d);
    }
}

static void text(lv_obj_t *canvas, int x, int y, int w, const char *s,
                 const lv_font_t *font, lv_text_align_t align, bool invert) {
    lv_draw_label_dsc_t d;
    init_label_dsc(&d, invert ? LVGL_BACKGROUND : LVGL_FOREGROUND, font, align);
    lv_canvas_draw_text(canvas, x, y, w, &d, s);
}

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    fill(canvas, LVGL_BACKGROUND);

    bool critical = state->battery < 20;
    if (critical) {
        box(canvas, 0, 0, 68, 15, true);
        text(canvas, 2, 1, 64, "WARNING", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER, true);
    } else {
        text(canvas, 1, 0, 42, "NERV", &lv_font_montserrat_16, LV_TEXT_ALIGN_LEFT, false);
        text(canvas, 43, 3, 23, state->usb_selected ? "UMB" : "A.T", &lv_font_unscii_8,
             LV_TEXT_ALIGN_RIGHT, false);
    }

    char pwr[16];
    snprintf(pwr, sizeof(pwr), "PWR %u%%", state->battery);
    text(canvas, 2, 18, 64, critical ? "POWER LOW" : pwr, &lv_font_unscii_8,
         LV_TEXT_ALIGN_LEFT, false);

    int segments = 8;
    int lit = (state->battery * segments + 99) / 100;
    for (int i = 0; i < segments; i++) {
        box(canvas, 2 + i * 8, 29, 6, 6, i < lit);
    }

    text(canvas, 2, 39, 64, "SYNC RATE", &lv_font_unscii_8, LV_TEXT_ALIGN_LEFT, false);
    char sync[16];
    snprintf(sync, sizeof(sync), "%03u WPM", state->wpm);
    text(canvas, 2, 49, 64, sync, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT, false);

    rotate_canvas(canvas, cbuf);
}

static void draw_middle(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);
    fill(canvas, LVGL_BACKGROUND);

    text(canvas, 2, 0, 64, "MAGI LINK", &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER, false);
    text(canvas, 2, 15, 64, state->usb_selected ? "UMBILICAL" : (state->connected ? "CONNECTED" : "STANDBY"),
         &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER, false);

    for (int i = 0; i < 5; i++) {
        bool selected = (!state->usb_selected && state->profile_index == i);
        int x = 2 + i * 13;
        box(canvas, x, 29, 11, 15, selected);
        char n[2] = {(char)('1' + i), '\0'};
        text(canvas, x, 31, 11, n, &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER, selected);
    }

    text(canvas, 2, 49, 64, state->usb_selected ? "EXT POWER" : (state->bonded ? "PILOT LINK" : "UNPAIRED"),
         &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER, false);
    text(canvas, 2, 58, 64, "MEL / BAL / CAS", &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER, false);

    rotate_canvas(canvas, cbuf);
}

static void draw_bottom(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);
    fill(canvas, LVGL_BACKGROUND);

    text(canvas, 2, 0, 64, "EVA-01", &lv_font_montserrat_16, LV_TEXT_ALIGN_CENTER, false);
    text(canvas, 2, 18, 64, "OPERATION MODE", &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER, false);

    box(canvas, 5, 31, 58, 20, true);
    text(canvas, 7, 33, 54, mode_name(state->layer_index), &lv_font_montserrat_14,
         LV_TEXT_ALIGN_CENTER, true);

    text(canvas, 2, 55, 64, "NERV // TOKYO-3", &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER, false);
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

static void set_output_status(struct zmk_widget_status *widget, const struct output_status_state *state) {
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
    widget->state.wpm = state.wpm;
    draw_top(widget->obj, widget->cbuf, &widget->state);
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
