#pragma once
#include <lvgl.h>
#include <zephyr/kernel.h>
#include "util.h"

/* ZMK key aliases use short macros such as A, D, E, DOT, etc. The
 * NERV footer's tiny bitmap font uses those names as local glyph names,
 * so undefine the aliases in this display-only translation unit. */
#undef A
#undef D
#undef E
#undef G
#undef H
#undef I
#undef L
#undef N
#undef O
#undef R
#undef S
#undef T
#undef V
#undef W
#undef DOT

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[CANVAS_SIZE * CANVAS_SIZE];
    lv_color_t cbuf2[CANVAS_SIZE * CANVAS_SIZE];
    lv_color_t cbuf3[CANVAS_SIZE * CANVAS_SIZE];
    struct status_state state;
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);
