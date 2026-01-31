# SelfWidget — Status Now

Current state of the project: what has been done and what features exist.

---

## What We Have Done So Far

### Backend (WidgetAPI — Django)

- **Django + DRF setup** — REST API for server-driven UI
- **Models** — `WidgetType`, `Screen`, `WidgetInstance`, `DeviceSettings` (singleton)
- **Fetchers** — Strategy-based data providers for widget types:
  - `WeatherFetcher` — weather data (mock/API)
  - `StockFetcher` — stock data
  - `SystemStatsFetcher` — CPU, memory, disk stats
- **API endpoints**:
  - `GET /api/screens/` — Screens with widgets (configuration + data)
  - `GET /api/device-settings/` — Device settings (poll interval, brightness, etc.)
- **Seed command** — `python manage.py seed_widgets` for sample data
- **Admin** — Django admin for managing WidgetTypes, Screens, WidgetInstances, DeviceSettings
- **URL config** — Root redirects to `/api/screens/`

### Firmware (ESPWidget — ESP32 CYD)

- **Display** — Adafruit ILI9341 (320×240) with LVGL 8.3
- **Touch** — XPT2046 resistive touch driver
- **Pinout** — CYD board pins in `include/config/pins.h`
- **WiFi** — Connects using `wifi_config.h` (or defaults in code)
- **HTTP client** — Fetches `GET /api/screens/` at startup
- **WidgetFactory** — Builds LVGL widgets from JSON in a 2×2 grid
- **Main loop** — LVGL tick and timer handling with touch input

### Configuration

- **WiFi** — `include/config/wifi_config.h` (SSID, password, API base URL)
- **PlatformIO** — Build and upload for ESP32 CYD
- **Dependencies** — `requirements.txt` (Django, DRF, CORS, Redis, Gunicorn)

---

## Features

### Widget Types (implemented)

| Slug            | Display Name | Configuration  | Data Fields                          |
|-----------------|-------------|----------------|--------------------------------------|
| `weather-card`  | Weather     | city, unit     | temperature, condition, humidity     |
| `stock-card`    | Stock       | symbol         | symbol, price, change                |
| `system-stats`  | System Stats| —              | cpu_percent, memory_percent, disk_percent |

### Screens and Layout

- **Screens** — Named pages with `index` for ordering
- **2×2 grid** — Four positions (0–3): top-left, top-right, bottom-left, bottom-right
- **Position index** — Each WidgetInstance has `position_index` for grid placement

### Device Settings (model only, not yet used on ESP)

- `wifi_ssids` — List of `{ssid, password}` objects
- `poll_interval_sec` — Refresh interval (default 30)
- `brightness` — 0–100 (default 100)

### Fallbacks

- **WiFi failure** — Placeholder UI message
- **API fetch failure** — Placeholder UI message
- **No screens/widgets** — Label: "No screens — Configure backend — GET /api/screens/"
- **Unknown widget type** — Label: "Unknown: &lt;slug&gt;"

---

## Architecture

```
WidgetAPI (Django)                    ESP32 CYD (PlatformIO)
┌─────────────────────┐               ┌──────────────────────┐
│ GET /api/screens/   │◄──────────────│ WiFi + HTTPClient    │
│ GET /api/device-... │               │ ArduinoJson parse    │
│                     │               │ build_ui_from_screens│
│ Fetchers (registry) │               │ WidgetFactory        │
│ → Weather, Stock,   │               │ → LVGL widgets       │
│   SystemStats       │               │ 2×2 grid             │
└─────────────────────┘               └──────────────────────┘
```

---

## Not Yet Implemented / Future Work

- **Polling** — ESP fetches only at startup; no periodic refresh
- **Device settings usage** — `poll_interval_sec` and `brightness` not applied on ESP
- **Multiple screens** — ESP uses first screen only
- **Touch calibration** — Basic mapping; proper calibration step not implemented
- **Screen navigation** — No swipe/tap to switch screens
