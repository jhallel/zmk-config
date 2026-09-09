#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/usb.h>
#include "peripheral_status.h"

LV_IMG_DECLARE(eva01);
LV_IMG_DECLARE(eva02);
LV_IMG_DECLARE(eva03);
LV_IMG_DECLARE(eva04);
LV_IMG_DECLARE(eva05);
LV_IMG_DECLARE(eva06);
LV_IMG_DECLARE(eva07);
LV_IMG_DECLARE(eva08);
LV_IMG_DECLARE(eva09);
LV_IMG_DECLARE(eva10);
LV_IMG_DECLARE(eva11);
LV_IMG_DECLARE(eva12);
LV_IMG_DECLARE(eva13);
LV_IMG_DECLARE(eva14);
LV_IMG_DECLARE(eva15);
LV_IMG_DECLARE(eva16);
LV_IMG_DECLARE(eva17);
LV_IMG_DECLARE(eva18);
LV_IMG_DECLARE(eva19);
LV_IMG_DECLARE(eva20);
LV_IMG_DECLARE(eva21);
LV_IMG_DECLARE(eva22);
LV_IMG_DECLARE(eva23);
LV_IMG_DECLARE(eva24);

/* 110 animation ticks at 200 ms each:
 * 1-8 main character portraits held for ~2 s each
 * 9 NERV held for ~1.6 s
 * 10-14 warning frames blink for ~2.4 s
 * 15-24 EVA mouth opens progressively over ~2 s
 */
static const lv_img_dsc_t *anim_imgs[] = {
    &eva01,&eva01,&eva01,&eva01,&eva01,&eva01,&eva01,&eva01,&eva01,&eva01,
    &eva02,&eva02,&eva02,&eva02,&eva02,&eva02,&eva02,&eva02,&eva02,&eva02,
    &eva03,&eva03,&eva03,&eva03,&eva03,&eva03,&eva03,&eva03,&eva03,&eva03,
    &eva04,&eva04,&eva04,&eva04,&eva04,&eva04,&eva04,&eva04,&eva04,&eva04,
    &eva05,&eva05,&eva05,&eva05,&eva05,&eva05,&eva05,&eva05,&eva05,&eva05,
    &eva06,&eva06,&eva06,&eva06,&eva06,&eva06,&eva06,&eva06,&eva06,&eva06,
    &eva07,&eva07,&eva07,&eva07,&eva07,&eva07,&eva07,&eva07,&eva07,&eva07,
    &eva08,&eva08,&eva08,&eva08,&eva08,&eva08,&eva08,&eva08,&eva08,&eva08,
    &eva09,&eva09,&eva09,&eva09,&eva09,&eva09,&eva09,&eva09,
    &eva10,&eva11,&eva12,&eva13,&eva14,&eva13,&eva12,&eva11,&eva10,&eva11,&eva12,&eva13,
    &eva15,&eva16,&eva17,&eva18,&eva19,&eva20,&eva21,&eva22,&eva23,&eva24
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
struct peripheral_status_state { bool connected; };

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t bg;
    init_rect_dsc(&bg, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &bg);
    draw_battery(canvas, state);
    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                        state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
    rotate_canvas(canvas, cbuf);
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

static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}
static void set_connection_status(struct zmk_widget_status *widget, struct peripheral_status_state state) {
    widget->state.connected = state.connected;
    draw_top(widget->obj, widget->cbuf, &widget->state);
}
static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *art = lv_animimg_create(widget->obj);
    lv_animimg_set_src(art, (const void **)anim_imgs, ARRAY_SIZE(anim_imgs));
    lv_animimg_set_duration(art, CONFIG_CUSTOM_ANIMATION_SPEED);
    lv_animimg_set_repeat_count(art, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(art);
    lv_obj_align(art, LV_ALIGN_TOP_LEFT, 0, 0);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_peripheral_status_init();
    return 0;
}
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
