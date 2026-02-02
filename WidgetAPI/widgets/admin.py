from django.contrib import admin
from .models import WidgetType, Screen, WidgetInstance, DeviceSettings, ActiveNotification


@admin.register(WidgetType)
class WidgetTypeAdmin(admin.ModelAdmin):
    list_display = ('slug', 'display_name')
    search_fields = ['slug', 'display_name']


class WidgetInstanceInline(admin.TabularInline):
    model = WidgetInstance
    extra = 0
    autocomplete_fields = ['widget_type']


@admin.register(Screen)
class ScreenAdmin(admin.ModelAdmin):
    list_display = ('name', 'index')
    search_fields = ['name']
    inlines = [WidgetInstanceInline]


@admin.register(WidgetInstance)
class WidgetInstanceAdmin(admin.ModelAdmin):
    list_display = ('screen', 'widget_type', 'position_index')
    list_filter = ('widget_type',)
    autocomplete_fields = ['screen', 'widget_type']


@admin.register(DeviceSettings)
class DeviceSettingsAdmin(admin.ModelAdmin):
    list_display = ('id', 'poll_interval_sec', 'brightness')

    def has_add_permission(self, request):
        return not DeviceSettings.objects.exists()

    def has_delete_permission(self, request, obj=None):
        return False


@admin.register(ActiveNotification)
class ActiveNotificationAdmin(admin.ModelAdmin):
    list_display = ('title', 'level', 'is_active', 'source_identifier', 'created_at')
    list_filter = ('level', 'is_active', 'source_identifier')
    search_fields = ('title', 'message')
