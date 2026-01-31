# WidgetAPI — Server-Driven UI Backend

Django + DRF backend for the ESP32 CYD smart display. The API returns screens and widgets with configuration plus real-time (or mock) data so the device renders exactly what the backend dictates.

## Setup

1. **Create and activate a virtual environment** (recommended):
   ```bash
   cd WidgetAPI
   python -m venv .venv
   source .venv/bin/activate   # Windows: .venv\Scripts\activate
   ```

2. **Install dependencies** (from repo root):
   ```bash
   pip install -r requirements.txt
   ```

3. **Run migrations**:
   ```bash
   cd WidgetAPI
   python manage.py migrate
   ```

4. **Seed sample data** (optional):
   ```bash
   python manage.py seed_widgets
   ```

## Run the server

From the `WidgetAPI` directory:

```bash
python manage.py runserver
```

Server runs at `http://127.0.0.1:8000/`.

## API

### GET /api/screens/

Returns a list of screens. Each screen includes its widgets with `configuration` and computed `data` (from the Fetcher for that widget type).

**Example request:**
```bash
curl http://127.0.0.1:8000/api/screens/
```

**Example response** (after seeding):
```json
[
  {
    "id": 1,
    "name": "Home",
    "index": 0,
    "widgets": [
      {
        "id": 1,
        "widget_type": { "id": 1, "slug": "weather-card", "display_name": "Weather" },
        "configuration": { "city": "Ankara", "unit": "C" },
        "position_index": 0,
        "data": { "temperature": 22, "unit": "C", "condition": "Sunny", "humidity": 45, "city": "Ankara" }
      }
    ]
  }
]
```

## Admin

Create a superuser and use the Django admin to manage WidgetTypes, Screens, WidgetInstances, and DeviceSettings:

```bash
python manage.py createsuperuser
# then open http://127.0.0.1:8000/admin/
```

## Project layout

- `config/` — Django project (settings, root urls, WSGI/ASGI).
- `widgets/` — App: models (WidgetType, Screen, WidgetInstance, DeviceSettings), serializers, views, URLs.
- `widgets/services/fetchers/` — Fetcher strategy: BaseFetcher, WeatherFetcher, StockFetcher, SystemStatsFetcher; registry by widget slug.
- `widgets/management/commands/seed_widgets.py` — Seeds default data for development.
