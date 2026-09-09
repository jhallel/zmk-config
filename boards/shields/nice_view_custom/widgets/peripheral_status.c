#include <zephyr/kernel.h>
#include <lvgl.h>
#include "peripheral_status.h"

LV_IMG_DECLARE(eva01); LV_IMG_DECLARE(eva02); LV_IMG_DECLARE(eva03); LV_IMG_DECLARE(eva04);
LV_IMG_DECLARE(eva05); LV_IMG_DECLARE(eva06); LV_IMG_DECLARE(eva07); LV_IMG_DECLARE(eva08);
LV_IMG_DECLARE(eva09); LV_IMG_DECLARE(eva10); LV_IMG_DECLARE(eva11); LV_IMG_DECLARE(eva12);
LV_IMG_DECLARE(eva13); LV_IMG_DECLARE(eva14); LV_IMG_DECLARE(eva15); LV_IMG_DECLARE(eva16);
LV_IMG_DECLARE(eva17); LV_IMG_DECLARE(eva18); LV_IMG_DECLARE(eva19); LV_IMG_DECLARE(eva20);
LV_IMG_DECLARE(eva21); LV_IMG_DECLARE(eva22); LV_IMG_DECLARE(eva23); LV_IMG_DECLARE(eva24);
LV_IMG_DECLARE(eva25); LV_IMG_DECLARE(eva26); LV_IMG_DECLARE(eva27); LV_IMG_DECLARE(eva28);
LV_IMG_DECLARE(eva29); LV_IMG_DECLARE(eva30); LV_IMG_DECLARE(eva31); LV_IMG_DECLARE(eva32);

/* 89 playback steps at 220 ms each (~19.58 s total). */
static const lv_img_dsc_t *anim_imgs[] = {
    &eva01,&eva01,&eva01,&eva01,
    &eva02,&eva02,&eva02,&eva02,
    &eva03,&eva03,&eva03,&eva03,&eva03,&eva04,
    &eva05,&eva05,&eva05,&eva05,&eva05,&eva06,
    &eva07,&eva07,&eva07,&eva07,&eva07,&eva08,
    &eva09,&eva09,&eva09,&eva09,&eva09,&eva10,
    &eva11,&eva11,&eva11,&eva11,&eva11,&eva12,
    &eva13,&eva13,&eva13,&eva13,&eva13,&eva14,
    &eva15,&eva15,&eva15,&eva15,&eva15,&eva16,
    &eva17,&eva17,&eva17,&eva17,&eva17,
    &eva18,&eva18,&eva18,&eva18,&eva18,
    &eva19,&eva19,
    &eva20,&eva21,&eva20,&eva21,&eva20,&eva21,&eva22,&eva23,&eva22,&eva23,&eva24,&eva23,
    &eva25,&eva25,&eva25,&eva26,&eva26,&eva27,&eva28,&eva29,&eva30,&eva31,
    &eva32,&eva32,&eva31,&eva32,&eva19
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    lv_obj_t *art = lv_animimg_create(widget->obj);
    lv_obj_align(art, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_animimg_set_src(art, (const void **)anim_imgs, ARRAY_SIZE(anim_imgs));
    lv_animimg_set_duration(art, CONFIG_CUSTOM_ANIMATION_SPEED);
    lv_animimg_set_repeat_count(art, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(art);
    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
