from .base import BaseFetcher
from .beszel import BeszelFetcher


# Source tag so we can deactivate only Beszel-related notifications when recovered
BESZEL_SOURCE = 'beszel'

# Default sound for alert: volume 100%, 2500 Hz
ALERT_VOLUME = 100
ALERT_FREQUENCY = 2500
ALERT_DURATION_MS = 500


class SystemStatsFetcher(BaseFetcher):
    """
    System stats for homelab display.
    When url, identity, and password are configured, fetches real data from Beszel.
    Otherwise returns mock data.
    On Beszel error: updates ActiveNotification (for /api/notifications/current/).
    On success: deactivates the Beszel-related ActiveNotification.
    """

    _MOCK_DATA = {
        'cpu_percent': 24,
        'memory_percent': 62,
        'temp': 45,
    }

    def fetch(self, configuration: dict) -> dict:
        from widgets.models import ActiveNotification, NotificationLevel

        config = configuration or {}
        url = (config.get('url') or '').strip().rstrip('/')
        identity = config.get('identity') or config.get('email', '')
        password = config.get('password', '')

        if url and identity and password:
            result = BeszelFetcher().fetch(config)
            if 'error' in result:
                error_msg = result.get('error', 'Connection failed')
                message = f'Beszel: {error_msg}'
                obj = ActiveNotification.objects.filter(
                    source_identifier=BESZEL_SOURCE,
                ).order_by('-created_at').first()
                if obj:
                    obj.title = 'Server Alert'
                    obj.message = message
                    obj.level = NotificationLevel.CRITICAL
                    obj.is_active = True
                    obj.sound_enabled = True
                    obj.volume = ALERT_VOLUME
                    obj.duration_ms = ALERT_DURATION_MS
                    obj.frequency = ALERT_FREQUENCY
                    obj.save()
                else:
                    ActiveNotification.objects.create(
                        source_identifier=BESZEL_SOURCE,
                        title='Server Alert',
                        message=message,
                        level=NotificationLevel.CRITICAL,
                        is_active=True,
                        sound_enabled=True,
                        volume=ALERT_VOLUME,
                        duration_ms=ALERT_DURATION_MS,
                        frequency=ALERT_FREQUENCY,
                    )
                return {
                    'cpu_percent': 0,
                    'memory_percent': 0,
                    'temp': 0,
                }
            # Recovered: deactivate Beszel notification
            ActiveNotification.objects.filter(
                source_identifier=BESZEL_SOURCE,
                is_active=True,
            ).update(is_active=False)
            # Map Beszel {cpu, ram, temp} -> ESP expects {cpu_percent, memory_percent, temp}
            return {
                'cpu_percent': int(round(result.get('cpu', 0))),
                'memory_percent': int(round(result.get('ram', 0))),
                'temp': int(round(result.get('temp', 0))),
            }
        return self._MOCK_DATA.copy()
