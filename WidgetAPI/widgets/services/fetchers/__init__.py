from .base import BaseFetcher
from .weather import WeatherFetcher
from .stock import StockFetcher
from .system_stats import SystemStatsFetcher

# Registry: widget_type slug -> Fetcher class (instantiate per call)
FETCHER_REGISTRY = {
    'weather-card': WeatherFetcher,
    'stock-card': StockFetcher,
    'system-stats': SystemStatsFetcher,
}


def get_fetcher(slug: str):
    """Return fetcher instance for slug, or None if not registered."""
    fetcher_class = FETCHER_REGISTRY.get(slug)
    if fetcher_class is None:
        return None
    return fetcher_class()


__all__ = ['BaseFetcher', 'WeatherFetcher', 'StockFetcher', 'SystemStatsFetcher', 'FETCHER_REGISTRY', 'get_fetcher']
