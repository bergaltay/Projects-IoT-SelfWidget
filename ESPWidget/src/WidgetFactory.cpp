/**
 * WidgetFactory - Build LVGL widgets from JSON in 2x2 grid.
 */
#include "WidgetFactory.h"
#include "lvgl.h"
#include <stdio.h>

static void get_cell_bounds(lv_obj_t *parent, int position_index, lv_coord_t *x, lv_coord_t *y, lv_coord_t *w, lv_coord_t *h) {
    lv_coord_t pw = lv_obj_get_content_width(parent);
    lv_coord_t ph = lv_obj_get_content_height(parent);
    *w = pw / 2;
    *h = ph / 2;
    switch (position_index) {
        case 0: *x = 0;          *y = 0;          break; /* top-left */
        case 1: *x = *w;        *y = 0;          break; /* top-right */
        case 2: *x = 0;         *y = *h;         break; /* bottom-left */
        case 3: *x = *w;        *y = *h;         break; /* bottom-right */
        default: *x = 0; *y = 0; break;
    }
}

static lv_obj_t *make_cell_container(lv_obj_t *parent, int position_index) {
    lv_coord_t x, y, w, h;
    get_cell_bounds(parent, position_index, &x, &y, &w, &h);
    lv_obj_t *cont = lv_obj_create(parent);
    /* Full cell size: no gaps, widgets fill the screen */
    lv_obj_set_size(cont, w, h);
    lv_obj_set_pos(cont, x, y);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(cont, 1, 0);
    lv_obj_set_style_border_color(cont, lv_color_hex(0x475569), 0);
    lv_obj_set_style_pad_all(cont, 6, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    return cont;
}

static void build_weather_card(lv_obj_t *cont, JsonObject data) {
    const char *city = data["city"].as<const char *>();
    if (!city) city = "?";
    int temp = data["temperature"] | 0;
    const char *unit = data["unit"].as<const char *>();
    if (!unit) unit = "C";
    const char *cond = data["condition"].as<const char *>();
    if (!cond) cond = "?";

    char buf[64];
    snprintf(buf, sizeof(buf), "%s", city);
    lv_obj_t *lbl_city = lv_label_create(cont);
    lv_label_set_text(lbl_city, buf);
    lv_obj_set_style_text_font(lbl_city, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_city, lv_color_hex(0xcbd5e1), 0);
    lv_obj_align(lbl_city, LV_ALIGN_TOP_MID, 0, 4);

    snprintf(buf, sizeof(buf), "%d °%s", temp, unit);
    lv_obj_t *lbl_temp = lv_label_create(cont);
    lv_label_set_text(lbl_temp, buf);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_temp, lv_color_hex(0xf8fafc), 0);
    lv_obj_center(lbl_temp);

    snprintf(buf, sizeof(buf), "%s", cond);
    lv_obj_t *lbl_cond = lv_label_create(cont);
    lv_label_set_text(lbl_cond, buf);
    lv_obj_set_style_text_font(lbl_cond, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_cond, lv_color_hex(0xcbd5e1), 0);
    lv_obj_align(lbl_cond, LV_ALIGN_BOTTOM_MID, 0, -4);
}

static void build_stock_card(lv_obj_t *cont, JsonObject data) {
    const char *symbol = data["symbol"].as<const char *>();
    if (!symbol) symbol = "?";
    double price = data["price"] | 0.0;
    double chg = data["change"] | 0.0;

    char buf[64];
    snprintf(buf, sizeof(buf), "%s", symbol);
    lv_obj_t *lbl_sym = lv_label_create(cont);
    lv_label_set_text(lbl_sym, buf);
    lv_obj_set_style_text_font(lbl_sym, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_sym, lv_color_hex(0xcbd5e1), 0);
    lv_obj_align(lbl_sym, LV_ALIGN_TOP_MID, 0, 4);

    snprintf(buf, sizeof(buf), "%.2f", price);
    lv_obj_t *lbl_price = lv_label_create(cont);
    lv_label_set_text(lbl_price, buf);
    lv_obj_set_style_text_font(lbl_price, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_price, lv_color_hex(0xf8fafc), 0);
    lv_obj_center(lbl_price);

    snprintf(buf, sizeof(buf), "%+.2f", chg);
    lv_obj_t *lbl_chg = lv_label_create(cont);
    lv_label_set_text(lbl_chg, buf);
    lv_obj_set_style_text_font(lbl_chg, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_chg, chg >= 0 ? lv_color_hex(0x22c55e) : lv_color_hex(0xef4444), 0);
    lv_obj_align(lbl_chg, LV_ALIGN_BOTTOM_MID, 0, -4);
}

static void build_system_stats(lv_obj_t *cont, JsonObject data) {
    int cpu = data["cpu_percent"] | 0;
    int mem = data["memory_percent"] | 0;
    int disk = data["disk_percent"] | 0;

    char buf[48];
    snprintf(buf, sizeof(buf), "CPU: %d%%", cpu);
    lv_obj_t *lbl_cpu = lv_label_create(cont);
    lv_label_set_text(lbl_cpu, buf);
    lv_obj_set_style_text_font(lbl_cpu, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_cpu, lv_color_hex(0xf8fafc), 0);
    lv_obj_align(lbl_cpu, LV_ALIGN_TOP_LEFT, 6, 6);

    snprintf(buf, sizeof(buf), "Mem: %d%%", mem);
    lv_obj_t *lbl_mem = lv_label_create(cont);
    lv_label_set_text(lbl_mem, buf);
    lv_obj_set_style_text_font(lbl_mem, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_mem, lv_color_hex(0xf8fafc), 0);
    lv_obj_align(lbl_mem, LV_ALIGN_LEFT_MID, 6, 0);

    snprintf(buf, sizeof(buf), "Disk: %d%%", disk);
    lv_obj_t *lbl_disk = lv_label_create(cont);
    lv_label_set_text(lbl_disk, buf);
    lv_obj_set_style_text_font(lbl_disk, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_disk, lv_color_hex(0xf8fafc), 0);
    lv_obj_align(lbl_disk, LV_ALIGN_BOTTOM_LEFT, 6, -6);
}

void WidgetFactory::createWidget(
    lv_obj_t *parent,
    int position_index,
    const char *type,
    JsonObject data,
    JsonObject config
) {
    (void)config;
    if (position_index < 0 || position_index > 3) return;

    lv_obj_t *cont = make_cell_container(parent, position_index);

    if (strcmp(type, "weather-card") == 0) {
        build_weather_card(cont, data);
    } else if (strcmp(type, "stock-card") == 0) {
        build_stock_card(cont, data);
    } else if (strcmp(type, "system-stats") == 0) {
        build_system_stats(cont, data);
    } else {
        lv_obj_t *lbl = lv_label_create(cont);
        lv_label_set_text_fmt(lbl, "Unknown: %s", type ? type : "");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xcbd5e1), 0);
        lv_obj_center(lbl);
    }
}
