"""
Weather fetcher via Open-Meteo API (no API key required).
"""
import requests

from .base import BaseFetcher

OPEN_METEO_URL = 'https://api.open-meteo.com/v1/forecast'
DEFAULT_LAT = 39.93
DEFAULT_LON = 32.85
DEFAULT_CITY = 'Ankara, Maltepe'
TIMEOUT = 10

# WMO weather code -> human-readable condition
WMO_CONDITIONS = {
    0: 'Sunny',
    **{i: 'Cloudy' for i in (1, 2, 3)},
    45: 'Foggy',
    48: 'Foggy',
    **{i: 'Rain' for i in range(51, 68)},
    **{i: 'Rain' for i in range(80, 83)},
    **{i: 'Snow' for i in range(71, 78)},
    **{i: 'Snow' for i in (85, 86)},
    **{i: 'Storm' for i in range(95, 100)},
}


def _wmo_to_condition(code: int) -> str:
    """Map WMO weather code to condition text."""
    return WMO_CONDITIONS.get(code, 'Unknown')


class WeatherFetcher(BaseFetcher):
    """
    Fetches weather from Open-Meteo API.
    Configuration: {"latitude": 39.93, "longitude": 32.85, "city": "...", "unit": "C"}
    If empty, defaults to Ankara, Maltepe (39.93, 32.85).
    """

    def fetch(self, configuration: dict) -> dict:
        config = configuration or {}
        lat = config.get('latitude')
        lon = config.get('longitude')
        if lat is None:
            lat = DEFAULT_LAT
        if lon is None:
            lon = DEFAULT_LON
        city = config.get('city') or DEFAULT_CITY
        unit = config.get('unit', 'C')

        try:
            resp = requests.get(
                OPEN_METEO_URL,
                params={
                    'latitude': lat,
                    'longitude': lon,
                    'current': 'temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m',
                },
                timeout=TIMEOUT,
            )
            resp.raise_for_status()
            data = resp.json()
        except requests.RequestException:
            return {
                'temperature': 0,
                'unit': unit,
                'condition': 'Unknown',
                'humidity': 0,
                'wind': 0,
                'city': city,
            }

        current = data.get('current') or {}
        temp = current.get('temperature_2m')
        humidity = current.get('relative_humidity_2m')
        wind = current.get('wind_speed_10m')
        wmo_code = current.get('weather_code', 0)

        return {
            'temperature': int(round(temp)) if temp is not None else 0,
            'unit': unit,
            'condition': _wmo_to_condition(int(wmo_code)),
            'humidity': humidity if humidity is not None else 0,
            'wind': wind if wind is not None else 0,
            'city': city,
        }
