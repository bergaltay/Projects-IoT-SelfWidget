from .base import BaseFetcher


class WeatherFetcher(BaseFetcher):
    """Mock weather data; uses configuration city and unit."""

    def fetch(self, configuration: dict) -> dict:
        city = configuration.get('city', 'Unknown')
        unit = configuration.get('unit', 'C')
        # Mock data
        temp = 22 if unit == 'C' else 72
        return {
            'temperature': temp,
            'unit': unit,
            'condition': 'Sunny',
            'humidity': 45,
            'city': city,
        }
