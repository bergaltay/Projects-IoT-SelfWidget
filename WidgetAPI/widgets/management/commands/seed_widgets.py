"""
Management command to seed default WidgetTypes, Screen, WidgetInstances, and DeviceSettings.
Run: python manage.py seed_widgets
"""
from django.core.management.base import BaseCommand
from widgets.models import WidgetType, Screen, WidgetInstance, DeviceSettings


class Command(BaseCommand):
    help = 'Seed default widget types, one screen with sample widgets, and device settings.'

    def handle(self, *args, **options):
        # WidgetTypes
        types_data = [
            {'slug': 'weather-card', 'display_name': 'Weather'},
            {'slug': 'stock-card', 'display_name': 'Stock'},
            {'slug': 'system-stats', 'display_name': 'System Stats'},
        ]
        for d in types_data:
            wt, created = WidgetType.objects.get_or_create(slug=d['slug'], defaults=d)
            self.stdout.write(f'  WidgetType {wt.slug}: {"created" if created else "exists"}')

        # Screen
        screen, created = Screen.objects.get_or_create(
            index=0,
            defaults={'name': 'Home'}
        )
        if created:
            self.stdout.write('  Screen "Home" (index=0) created.')
        else:
            screen.name = 'Home'
            screen.save()

        # WidgetInstances (2x2 grid: position_index 0,1,2,3)
        weather_type = WidgetType.objects.get(slug='weather-card')
        stock_type = WidgetType.objects.get(slug='stock-card')
        system_type = WidgetType.objects.get(slug='system-stats')

        instances_data = [
            (0, weather_type, {'city': 'Ankara', 'unit': 'C'}),
            (1, stock_type, {'symbol': 'AAPL'}),
            (2, system_type, {}),
            (3, weather_type, {'city': 'Istanbul', 'unit': 'C'}),
        ]
        for pos, wtype, config in instances_data:
            wi, created = WidgetInstance.objects.get_or_create(
                screen=screen,
                position_index=pos,
                defaults={'widget_type': wtype, 'configuration': config}
            )
            if not created:
                wi.widget_type = wtype
                wi.configuration = config
                wi.save()
            self.stdout.write(f'  WidgetInstance position {pos} ({wtype.slug}): {"created" if created else "updated"}')

        # DeviceSettings singleton
        DeviceSettings.objects.get_or_create_singleton(
            wifi_ssids=[],
            poll_interval_sec=30,
            brightness=100,
        )
        self.stdout.write('  DeviceSettings singleton ensured.')

        self.stdout.write(self.style.SUCCESS('Seed completed. GET /api/screens/ will return sample data.'))
