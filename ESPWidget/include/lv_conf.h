/**
 * LVGL 8.3 configuration for ESP32 CYD (ILI9341 320x240, RGB565).
 * Place in include/ and use build_flags so LVGL finds it.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Enable lv_conf.h */
#if 1

/* Color depth: 16 bit for ILI9341 RGB565 */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/* Resolution (CYD typical) */
#define LV_DISPLAY_HOR_RES 320
#define LV_DISPLAY_VER_RES 240

/* Draw buffer: ~1/10 screen to save RAM */
#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_DEF_REFR_PERIOD 30

/* Input device: one touchpad */
#define LV_INDEV_DEF_READ_PERIOD 30

/* Misc */
#define LV_MEM_CUSTOM 0
#define LV_USE_LOG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_GPU 0
#define LV_USE_FILESYSTEM 0
#define LV_USE_OS 0
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_USE_ANIMATION 1
#define LV_USE_SHADOW 0
#define LV_USE_BLEND_MODES 1
#define LV_USE_OPA_SCALE 0
#define LV_USE_IMG_TRANSFORM 0
#define LV_USE_GROUP 1
#define LV_USE_GPU_STM32_DMA2D 0
#define LV_USE_SNAPSHOT 0
#define LV_USE_MONKEY 0
#define LV_USE_GRIDNAV 0
#define LV_USE_FRAGMENT 0
#define LV_USE_FLEX 1

#endif /* enable */

#endif /* LV_CONF_H */
