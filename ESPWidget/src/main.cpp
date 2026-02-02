/**
 * ESPWidget - Server-driven UI for ESP32 CYD.
 * Display: Adafruit ILI9341 (my_disp_flush). Touch: XPT2046 (my_touchpad_read).
 */
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <stdio.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "config/pins.h"
#include "WidgetFactory.h"
#include "lvgl.h"

/* ----- Display (ILI9341) ----- */
static Adafruit_ILI9341 tft = Adafruit_ILI9341(
    TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO
);

#define TFT_W 320
#define TFT_H 240

static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[TFT_W * 10];
static lv_color_t buf_2[TFT_W * 10];

static void my_disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);
    tft.drawRGBBitmap((int16_t)area->x1, (int16_t)area->y1, (uint16_t *)color_map, w, h);
    lv_disp_flush_ready(drv);
}

/* ----- Touch (XPT2046) ----- */
static SPIClass touchSPI(HSPI);
static XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

/* Touch calibration: legacy CYD mapping (axes swapped, calibrated range 200–3700) */
#define TOUCH_MAP_LO  200
#define TOUCH_MAP_HI  3700

static void my_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    if (touch.touched()) {
        data->state = LV_INDEV_STATE_PRESSED;
        TS_Point p = touch.getPoint();
        /* Legacy logic: X uses p.y, Y uses p.x (axes swapped); map 200–3700 -> 0–width/height
         * If the screen feels upside-down (top and bottom swapped), invert Y by reversing
         * the output range so moving down increases Y correctly. */
        int32_t touchX = (int32_t)map(p.y, TOUCH_MAP_LO, TOUCH_MAP_HI, 0, TFT_W);
        int32_t touchY = (int32_t)map(p.x, TOUCH_MAP_LO, TOUCH_MAP_HI, TFT_H - 1, 0);
        touchX = (int32_t)constrain(touchX, 0, TFT_W - 1);
        touchY = (int32_t)constrain(touchY, 0, TFT_H - 1);
        data->point.x = touchX;
        data->point.y = touchY;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ----- WiFi / API config: copy include/config/wifi_config.h.example to wifi_config.h ----- */
#ifdef __has_include
#  if __has_include("config/wifi_config.h")
#    include "config/wifi_config.h"
#  endif
#endif
#ifndef WIFI_SSID
#define WIFI_SSID "YourSSID"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "YourPassword"
#endif
#ifndef API_BASE_URL
#define API_BASE_URL "http://192.168.1.100:8000"
#endif

#define DEVICE_NAME       "HomeLab"
#define STATUS_BAR_H      24
#define TZ_OFFSET_SEC     (3 * 3600)   /* UTC+3 Turkey; set 0 for UTC */
#define NTP_SERVER1       "pool.ntp.org"
#define NTP_SERVER2       "time.nist.gov"

/* Backlight PWM (TFT_BL pin 21): 0-100%, default 100% */
#define LEDC_CHANNEL_BL  0
#define LEDC_FREQ_BL     5000
#define LEDC_RESOLUTION  8
static int g_brightness = 100;

static void set_backlight_pct(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    g_brightness = pct;
    uint32_t duty = (uint32_t)(pct * 255) / 100;
    ledcWrite(LEDC_CHANNEL_BL, duty);
}

#define SPI_DISPLAY_FREQ  27000000

static void init_display() {
    ledcSetup(LEDC_CHANNEL_BL, LEDC_FREQ_BL, LEDC_RESOLUTION);
    ledcAttachPin(TFT_BL, LEDC_CHANNEL_BL);
    ledcWrite(LEDC_CHANNEL_BL, 255);
    tft.begin();
#if defined(ESP32)
    SPI.setFrequency(SPI_DISPLAY_FREQ);
#endif
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
}

static void init_touch() {
    touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touch.begin(touchSPI);
    touch.setRotation(0);
}

static void init_lvgl() {
    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, TFT_W * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_W;
    disp_drv.ver_res = TFT_H;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

/* Status bar and content area; set by create_main_layout(), used by timers */
static lv_obj_t *g_content_area = NULL;
static lv_obj_t *g_status_wifi_label = NULL;
static lv_obj_t *g_status_clock_label = NULL;

static void settings_btn_cb(lv_event_t *e);

static lv_obj_t *create_main_layout(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e293b), 0);
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_set_style_pad_column(scr, 0, 0);

    /* Status bar: fixed height, dark background */
    lv_obj_t *status_bar = lv_obj_create(scr);
    lv_obj_set_height(status_bar, STATUS_BAR_H);
    lv_obj_set_width(status_bar, lv_pct(100));
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_pad_all(status_bar, 2, 0);
    lv_obj_set_style_flex_grow(status_bar, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Left: device name */
    lv_obj_t *name_lbl = lv_label_create(status_bar);
    lv_label_set_text(name_lbl, DEVICE_NAME);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(name_lbl, lv_color_hex(0xf1f5f9), 0);

    /* Right: WiFi icon + clock (container so they stay together) */
    lv_obj_t *right_cont = lv_obj_create(status_bar);
    lv_obj_set_height(right_cont, STATUS_BAR_H - 4);
    lv_obj_set_style_bg_opa(right_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_cont, 0, 0);
    lv_obj_set_style_pad_all(right_cont, 0, 0);
    lv_obj_clear_flag(right_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(right_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right_cont, 6, 0);

    g_status_wifi_label = lv_label_create(right_cont);
    lv_label_set_text(g_status_wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(g_status_wifi_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_status_wifi_label, lv_color_hex(0x888888), 0);

    g_status_clock_label = lv_label_create(right_cont);
    lv_label_set_text(g_status_clock_label, "--:--");
    lv_obj_set_style_text_font(g_status_clock_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_status_clock_label, lv_color_hex(0xf1f5f9), 0);

    /* Settings button (rightmost): opens settings modal */
    lv_obj_t *settings_btn = lv_btn_create(right_cont);
    lv_obj_set_size(settings_btn, 28, 20);
    lv_obj_set_style_pad_all(settings_btn, 2, 0);
    lv_obj_add_event_cb(settings_btn, settings_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *settings_lbl = lv_label_create(settings_btn);
    lv_label_set_text(settings_lbl, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_lbl);
    lv_obj_set_style_text_font(settings_lbl, &lv_font_montserrat_14, 0);

    /* Content area: flex-grow 1, takes remaining space */
    g_content_area = lv_obj_create(scr);
    lv_obj_set_width(g_content_area, lv_pct(100));
    lv_obj_set_style_flex_grow(g_content_area, 1, 0);
    lv_obj_set_style_bg_color(g_content_area, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_width(g_content_area, 0, 0);
    lv_obj_set_style_pad_all(g_content_area, 0, 0);
    lv_obj_clear_flag(g_content_area, LV_OBJ_FLAG_SCROLLABLE);

    return g_content_area;
}

static void clock_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (!g_status_clock_label) return;
    time_t now = time(nullptr);
    if (now < 0) return;
    struct tm *t = localtime(&now);
    if (!t) return;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
    lv_label_set_text(g_status_clock_label, buf);
}

static void wifi_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (!g_status_wifi_label) return;
    if (WiFi.status() != WL_CONNECTED) {
        lv_obj_set_style_text_color(g_status_wifi_label, lv_color_hex(0x64748b), 0);
        return;
    }
    int rssi = WiFi.RSSI();
    if (rssi > -50) {
        lv_obj_set_style_text_color(g_status_wifi_label, lv_color_hex(0x22c55e), 0);
    } else if (rssi > -70) {
        lv_obj_set_style_text_color(g_status_wifi_label, lv_color_hex(0xeab308), 0);
    } else {
        lv_obj_set_style_text_color(g_status_wifi_label, lv_color_hex(0xf97316), 0);
    }
}

static String fetch_screens_json() {
    HTTPClient http;
    String url = String(API_BASE_URL) + "/api/screens/";
    http.begin(url);
    int code = http.GET();
    String payload;
    if (code == HTTP_CODE_OK) {
        payload = http.getString();
    }
    http.end();
    return payload;
}

static void show_settings_modal();

static void modal_close_btn_cb(lv_event_t *e) {
    lv_obj_t *overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if (overlay) lv_obj_del(overlay);
}

static void modal_slider_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int val = (int)lv_slider_get_value(slider);
    set_backlight_pct(val);
}

static void show_settings_modal() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *overlay = lv_obj_create(scr);
    lv_obj_set_size(overlay, TFT_W, TFT_H);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 8, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_move_foreground(overlay);

    /* Fixed full-screen settings box, no scrolling */
    lv_obj_t *box = lv_obj_create(overlay);
    lv_obj_set_size(box, TFT_W - 16, TFT_H - 16);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0x475569), 0);
    lv_obj_set_style_pad_all(box, 10, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    lv_obj_t *title = lv_label_create(box);
    lv_label_set_text(title, "Device Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xf8fafc), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_clear_flag(title, LV_OBJ_FLAG_SCROLLABLE);

    char info_buf[192];
    int y_off = 32;
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(info_buf, sizeof(info_buf), "SSID: %s\nIP: %s",
                 WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    } else {
        snprintf(info_buf, sizeof(info_buf), "WiFi: Not connected");
    }
    lv_obj_t *info = lv_label_create(box);
    lv_label_set_text(info, info_buf);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(info, lv_color_hex(0xcbd5e1), 0);
    lv_obj_align(info, LV_ALIGN_TOP_LEFT, 4, y_off);
    lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);
    y_off += 36;

    lv_obj_t *api_lbl = lv_label_create(box);
    snprintf(info_buf, sizeof(info_buf), "API: %s", API_BASE_URL);
    lv_label_set_text(api_lbl, info_buf);
    lv_obj_set_style_text_font(api_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(api_lbl, lv_color_hex(0xcbd5e1), 0);
    lv_obj_align(api_lbl, LV_ALIGN_TOP_LEFT, 4, y_off);
    lv_obj_set_width(api_lbl, TFT_W - 40);
    lv_label_set_long_mode(api_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_clear_flag(api_lbl, LV_OBJ_FLAG_SCROLLABLE);
    y_off += 28;

    lv_obj_t *bright_lbl = lv_label_create(box);
    lv_label_set_text(bright_lbl, "Brightness");
    lv_obj_set_style_text_font(bright_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(bright_lbl, lv_color_hex(0xcbd5e1), 0);
    lv_obj_align(bright_lbl, LV_ALIGN_TOP_LEFT, 4, y_off);
    lv_obj_clear_flag(bright_lbl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *slider = lv_slider_create(box);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, g_brightness, LV_ANIM_OFF);
    lv_obj_set_width(slider, TFT_W - 48);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, y_off + 22);
    lv_obj_add_event_cb(slider, modal_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *close_btn = lv_btn_create(box);
    lv_obj_set_size(close_btn, 100, 36);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_add_event_cb(close_btn, modal_close_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_clear_flag(close_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "Close");
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xf8fafc), 0);
    lv_obj_center(close_lbl);
}

static void settings_btn_cb(lv_event_t *e) {
    (void)e;
    show_settings_modal();
}

#define TABVIEW_BAR_HEIGHT 24

static void build_ui_from_screens(lv_obj_t *content_parent, const String &json) {
    if (!content_parent) return;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.println("JSON parse failed");
        return;
    }
    if (doc.overflowed()) {
        Serial.println("JSON overflow");
        return;
    }
    JsonArray screens = doc.as<JsonArray>();
    if (screens.isNull() || screens.size() == 0) {
        Serial.println("No screens");
        return;
    }

    lv_obj_t *tabview = lv_tabview_create(content_parent, LV_DIR_BOTTOM, TABVIEW_BAR_HEIGHT);
    lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x1e293b), 0);
    lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);
    /* Disable touch swipe: navigation only via tab buttons */
    lv_obj_t *tabview_content = lv_tabview_get_content(tabview);
    lv_obj_clear_flag(tabview_content, LV_OBJ_FLAG_SCROLLABLE);

    for (JsonVariant screenV : screens) {
        JsonObject screenObj = screenV.as<JsonObject>();
        const char *name = screenObj["name"].as<const char *>();
        if (!name) name = "Screen";
        lv_obj_t *tab = lv_tabview_add_tab(tabview, name);
        lv_obj_set_style_bg_color(tab, lv_color_hex(0x1e293b), 0);
        lv_obj_update_layout(tabview);

        JsonArray widgets = screenObj["widgets"].as<JsonArray>();
        for (JsonVariant wv : widgets) {
            JsonObject w = wv.as<JsonObject>();
            int pos = w["position_index"] | 0;
            const char *type = w["widget_type"]["slug"].as<const char *>();
            if (!type) type = "";
            JsonObject data = w["data"].as<JsonObject>();
            JsonObject config = w["configuration"].as<JsonObject>();
            WidgetFactory::createWidget(tab, pos, type, data, config);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESPWidget starting");

    init_display();
    init_touch();
    init_lvgl();

    /* Root layout: flex column = status bar + content area */
    lv_obj_t *content_area = create_main_layout();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi connecting");
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi failed - showing placeholder UI");
    } else {
        Serial.print("WiFi OK: ");
        Serial.println(WiFi.localIP());
        configTime(TZ_OFFSET_SEC, 0, NTP_SERVER1, NTP_SERVER2);
        wifi_timer_cb(NULL);
        String json = fetch_screens_json();
        if (json.length() > 0) {
            lv_obj_update_layout(lv_scr_act());
            build_ui_from_screens(content_area, json);
        } else {
            Serial.println("API fetch failed - showing placeholder");
        }
    }
    if (g_content_area && lv_obj_get_child_cnt(g_content_area) == 0) {
        lv_obj_update_layout(lv_scr_act());
        lv_obj_t *label = lv_label_create(g_content_area);
        lv_label_set_text(label, "No screens\nConfigure backend\nGET /api/screens/");
        lv_obj_center(label);
    }

    lv_timer_create(clock_timer_cb, 1000, NULL);
    lv_timer_create(wifi_timer_cb, 5000, NULL);
}

void loop() {
    static uint32_t last_tick = 0;
    uint32_t now = millis();
    lv_tick_inc(now - last_tick);
    last_tick = now;
    lv_timer_handler();
    delay(5);
}
