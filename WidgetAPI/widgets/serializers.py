from rest_framework import serializers
from .models import WidgetType, Screen, WidgetInstance, DeviceSettings
from .services.fetchers import get_fetcher


class WidgetTypeSerializer(serializers.ModelSerializer):
    class Meta:
        model = WidgetType
        fields = ('id', 'slug', 'display_name')
        read_only_fields = ('id', 'slug', 'display_name')


class WidgetInstanceSerializer(serializers.ModelSerializer):
    widget_type = WidgetTypeSerializer(read_only=True)
    data = serializers.SerializerMethodField()

    class Meta:
        model = WidgetInstance
        fields = ('id', 'widget_type', 'configuration', 'position_index', 'data')
        read_only_fields = ('id', 'widget_type', 'configuration', 'position_index', 'data')

    def get_data(self, obj):
        fetcher = get_fetcher(obj.widget_type.slug)
        if fetcher is None:
            return None
        config = obj.configuration or {}
        try:
            return fetcher.fetch(config)
        except Exception:
            return None


class ScreenSerializer(serializers.ModelSerializer):
    widgets = WidgetInstanceSerializer(many=True, read_only=True, source='widget_instances')

    class Meta:
        model = Screen
        fields = ('id', 'name', 'index', 'widgets')
        read_only_fields = ('id', 'name', 'index', 'widgets')


class DeviceSettingsSerializer(serializers.ModelSerializer):
    class Meta:
        model = DeviceSettings
        fields = ('id', 'wifi_ssids', 'poll_interval_sec', 'brightness')
        read_only_fields = ('id', 'wifi_ssids', 'poll_interval_sec', 'brightness')
