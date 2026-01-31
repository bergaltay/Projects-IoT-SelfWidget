/**
 * ESPWidget - Server-driven UI for ESP32 CYD.
 * Display: Adafruit ILI9341 (my_disp_flush). Touch: XPT2046 (my_touchpad_read).
 */
#include <Arduino.h>
#include <WiFi.h>
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

static void my_touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    if (touch.touched()) {
        data->state = LV_INDEV_STATE_PRESSED;
        TS_Point p = touch.getPoint();
        /* Raw to display: XPT2046 often 0..4095; map to 0..TFT_W-1 and 0..TFT_H-1 */
        data->point.x = (int32_t)((uint32_t)p.x * TFT_W / 4096);
        data->point.y = (int32_t)((uint32_t)p.y * TFT_H / 4096);
        /* Some boards swap or invert axes */
        if (data->point.x < 0) data->point.x = 0;
        if (data->point.x >= TFT_W) data->point.x = TFT_W - 1;
        if (data->point.y < 0) data->point.y = 0;
        if (data->point.y >= TFT_H) data->point.y = TFT_H - 1;
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

static void init_display() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
}

static void init_touch() {
    touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touch.begin(touchSPI);
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

static void build_ui_from_screens(const String &json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.println("JSON parse failed");
        return;
    }
    JsonArray screens = doc.as<JsonArray>();
    if (screens.isNull() || screens.size() == 0) {
        Serial.println("No screens");
        return;
    }
    /* Use first screen only for this scaffold */
    JsonObject screenObj = screens[0].as<JsonObject>();
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x003366), 0);

    JsonArray widgets = screenObj["widgets"].as<JsonArray>();
    for (JsonVariant wv : widgets) {
        JsonObject w = wv.as<JsonObject>();
        int pos = w["position_index"] | 0;
        const char *type = w["widget_type"]["slug"].as<const char *>();
        if (!type) type = "";
        JsonObject data = w["data"].as<JsonObject>();
        JsonObject config = w["configuration"].as<JsonObject>();
        WidgetFactory::createWidget(scr, pos, type, data, config);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESPWidget starting");

    init_display();
    init_touch();
    init_lvgl();

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
        String json = fetch_screens_json();
        if (json.length() > 0) {
            build_ui_from_screens(json);
        } else {
            Serial.println("API fetch failed - showing placeholder");
        }
    }
    /* If no widgets were built, show a simple label */
    if (lv_obj_get_child_cnt(lv_scr_act()) == 0) {
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "No screens\nConfigure backend\nGET /api/screens/");
        lv_obj_center(label);
    }
}

void loop() {
    static uint32_t last_tick = 0;
    uint32_t now = millis();
    lv_tick_inc(now - last_tick);
    last_tick = now;
    lv_timer_handler();
    delay(5);
}
