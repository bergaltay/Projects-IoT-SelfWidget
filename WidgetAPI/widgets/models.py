from django.db import models


class WidgetType(models.Model):
    """Blueprint for a widget (e.g. weather-card, stock-card)."""
    slug = models.SlugField(max_length=64, unique=True)
    display_name = models.CharField(max_length=128)

    class Meta:
        ordering = ['slug']

    def __str__(self):
        return f'{self.display_name} ({self.slug})'


class Screen(models.Model):
    """One physical page on the device."""
    name = models.CharField(max_length=128)
    index = models.PositiveIntegerField(default=0, help_text='Order on device')

    class Meta:
        ordering = ['index']

    def __str__(self):
        return f'{self.name} (index={self.index})'


class WidgetInstance(models.Model):
    """Link between Screen and WidgetType with per-instance configuration."""
    screen = models.ForeignKey(Screen, on_delete=models.CASCADE, related_name='widget_instances')
    widget_type = models.ForeignKey(WidgetType, on_delete=models.PROTECT, related_name='instances')
    configuration = models.JSONField(default=dict, help_text='e.g. {"city": "Ankara", "unit": "C"}')
    position_index = models.PositiveIntegerField(default=0, help_text='Grid position (e.g. 0-3 for 2x2)')

    class Meta:
        ordering = ['screen', 'position_index']
        unique_together = [['screen', 'position_index']]

    def __str__(self):
        return f'{self.widget_type.slug} on {self.screen.name} @ {self.position_index}'


class DeviceSettingsManager(models.Manager):
    """Manager that enforces a single DeviceSettings row."""

    def get_or_create_singleton(self, **defaults):
        instance, _ = self.get_or_create(id=1, defaults=defaults)
        return instance

    def get_singleton(self):
        return self.get_or_create_singleton()


class DeviceSettings(models.Model):
    """Singleton config for the ESP32 (WiFi, poll interval, brightness)."""
    wifi_ssids = models.JSONField(
        default=list,
        help_text='List of {"ssid": "...", "password": "..."}',
    )
    poll_interval_sec = models.PositiveIntegerField(default=30)
    brightness = models.PositiveSmallIntegerField(default=100, help_text='0-100')

    objects = DeviceSettingsManager()

    class Meta:
        verbose_name = 'Device settings'
        verbose_name_plural = 'Device settings'

    def save(self, *args, **kwargs):
        self.id = 1
        super().save(*args, **kwargs)

    def __str__(self):
        return 'Device settings (singleton)'
