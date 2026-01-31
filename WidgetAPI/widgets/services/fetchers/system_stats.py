from .base import BaseFetcher


class SystemStatsFetcher(BaseFetcher):
    """Mock system stats (CPU, memory, disk) for homelab display."""

    def fetch(self, configuration: dict) -> dict:
        # Mock data
        return {
            'cpu_percent': 24,
            'memory_percent': 62,
            'disk_percent': 45,
        }
