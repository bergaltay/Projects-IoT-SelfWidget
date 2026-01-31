from .base import BaseFetcher


class StockFetcher(BaseFetcher):
    """Mock stock data; uses configuration symbol."""

    def fetch(self, configuration: dict) -> dict:
        symbol = configuration.get('symbol', 'UNKN')
        # Mock data
        return {
            'symbol': symbol,
            'price': 150.25,
            'change': 2.5,
            'change_percent': 1.69,
        }
