from .base import BaseFetcher
from .beszel import BeszelFetcher


class SystemStatsFetcher(BaseFetcher):
    """
    System stats for homelab display.
    When url, identity, and password are configured, fetches real data from Beszel.
    Otherwise returns mock data.
    """

    _MOCK_DATA = {
        'cpu_percent': 24,
        'memory_percent': 62,
        'temp': 45,
        'has_error': False,
    }

    def fetch(self, configuration: dict) -> dict:
        config = configuration or {}
        url = (config.get('url') or '').strip().rstrip('/')
        identity = config.get('identity') or config.get('email', '')
        password = config.get('password', '')

        if url and identity and password:
            result = BeszelFetcher().fetch(config)
            if 'error' in result:
                return {
                    'cpu_percent': 0,
                    'memory_percent': 0,
                    'temp': 0,
                    'has_error': True,
                }
            # Map Beszel {cpu, ram, temp} -> ESP expects {cpu_percent, memory_percent, temp}
            return {
                'cpu_percent': int(round(result.get('cpu', 0))),
                'memory_percent': int(round(result.get('ram', 0))),
                'temp': int(round(result.get('temp', 0))),
                'has_error': False,
            }
        return self._MOCK_DATA.copy()
