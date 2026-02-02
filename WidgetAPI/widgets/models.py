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


class NotificationLevel(models.TextChoices):
    INFO = 'INFO', 'Info'
    WARNING = 'WARNING', 'Warning'
    CRITICAL = 'CRITICAL', 'Critical'


class ActiveNotification(models.Model):
    """
    Currently active high-priority alert for the device.
    Fetchers (e.g. Beszel) create/update when there is an error; deactivate when recovered.
    ESP32 polls /api/notifications/current/ every few seconds.
    """
    title = models.CharField(max_length=128)
    message = models.TextField()
    level = models.CharField(max_length=16, choices=NotificationLevel.choices, default=NotificationLevel.CRITICAL)
    is_active = models.BooleanField(default=True)
    # Sound config for ESP32 buzzer
    sound_enabled = models.BooleanField(default=True)
    volume = models.PositiveSmallIntegerField(default=100, help_text='0-100')
    duration_ms = models.PositiveIntegerField(default=500)
    frequency = models.PositiveIntegerField(default=2500, help_text='Hz')
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)
    # Which source created this (e.g. 'beszel') so we can deactivate only that one
    source_identifier = models.CharField(max_length=64, default='', blank=True, db_index=True)

    class Meta:
        ordering = ['-created_at']
        verbose_name = 'Active notification'
        verbose_name_plural = 'Active notifications'

    def __str__(self):
        return f'{self.title} ({self.level})'
