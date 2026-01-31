# ESPWidget — Server-Driven UI Firmware (ESP32 CYD)

Firmware for the **ESP32-2432S028R (CYD)** smart display. The device acts as a thin client: it fetches screens and widget data from the WidgetAPI backend and renders them with LVGL on the ILI9341 display.

## Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- WidgetAPI backend running and reachable (e.g. `http://<host>:8000/api/screens/`)

## Pinout (CYD)

Display (ILI9341) and touch (XPT2046) pins are defined in [include/config/pins.h](include/config/pins.h):

| Function | Pin |
|----------|-----|
| TFT_BL   | 21  |
| TFT_CS   | 15  |
| TFT_DC   | 2   |
| TFT_MISO | 12  |
| TFT_MOSI | 13  |
| TFT_SCLK | 14  |
| TFT_RST  | -1  |
| XPT2046_IRQ | 36 |
| XPT2046_MOSI | 32 |
| XPT2046_MISO | 39 |
| XPT2046_CLK  | 25 |
| XPT2046_CS   | 33 |

## Build and upload

```bash
cd ESPWidget
pio run
pio run -t upload
pio device monitor -b 115200
```

## Configuration

1. **WiFi and API URL**  
   Copy [include/config/wifi_config.h.example](include/config/wifi_config.h.example) to `include/config/wifi_config.h` and set:
   - `WIFI_SSID`
   - `WIFI_PASS`
   - `API_BASE_URL` (e.g. `http://192.168.1.100:8000`)

   Do not commit `wifi_config.h` if it contains real credentials.

2. If `wifi_config.h` is not present, defaults in [src/main.cpp](src/main.cpp) are used (change them or add the include file).

## Behaviour

- **Setup:** Initializes display (Adafruit ILI9341), touch (XPT2046 on HSPI), LVGL display and input drivers, then connects to WiFi and requests `GET /api/screens/`.
- **UI:** Parses the JSON and, for the first screen, builds a 2x2 grid of widgets via `WidgetFactory` (weather-card, stock-card, system-stats). Each widget is placed by `position_index` (0–3).
- **Loop:** Runs `lv_tick_inc()` and `lv_timer_handler()` with a short delay.
- **Hidden Settings:** Tap the settings (gear) button in the status bar (right corner) to open the settings modal; see below.

## Hidden Settings Menu

A **Settings** button (gear icon) in the status bar (rightmost corner) opens a modal **Device Settings** window.

- **How to open:** Tap the settings button in the top-right of the status bar.
- **Contents:**
  - **Title:** "Device Settings"
  - **Info:** Current WiFi SSID and IP address (or "Not connected" if offline).
  - **Brightness slider:** 0–100%, controls the display backlight via PWM.
  - **Close button:** Dismisses the modal and returns to the main view.

**Hardware:** Backlight is on pin 21 (`TFT_BL`), controlled by ESP32 `ledc` (PWM); default 100%. Implemented in [src/main.cpp](src/main.cpp) (`show_settings_modal()`, settings button in status bar).

## Touch calibration

Touch coordinates are mapped from XPT2046 raw values (0–4095) to 320x240. If axes are swapped or inverted on your board, adjust the mapping in `my_touchpad_read()` in [src/main.cpp](src/main.cpp). A proper calibration step can be added later.

## Layout

- [platformio.ini](platformio.ini) — PlatformIO env and libs (Adafruit GFX/ILI9341, XPT2046, ArduinoJson 7, LVGL 8.3).
- [include/config/pins.h](include/config/pins.h) — CYD pin definitions.
- [include/lv_conf.h](include/lv_conf.h) — LVGL 8.3 config (color depth, fonts, etc.).
- [include/WidgetFactory.h](include/WidgetFactory.h), [src/WidgetFactory.cpp](src/WidgetFactory.cpp) — Build LVGL widgets from JSON (2x2 grid).
- [src/main.cpp](src/main.cpp) — Display/touch init, LVGL drivers, WiFi, HTTP, screen parsing, main loop, backlight PWM, hidden settings modal (long-press).
