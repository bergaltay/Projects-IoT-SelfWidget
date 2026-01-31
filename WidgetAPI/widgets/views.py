from rest_framework import generics
from rest_framework.views import APIView
from rest_framework.response import Response
from .models import Screen, DeviceSettings
from .serializers import ScreenSerializer, DeviceSettingsSerializer


class ScreenListView(generics.ListAPIView):
    """
    GET /api/screens/
    Returns list of screens with widgets; each widget has configuration + fetched data.
    """
    serializer_class = ScreenSerializer
    queryset = Screen.objects.all().order_by('index').prefetch_related('widget_instances__widget_type')


class DeviceSettingsView(APIView):
    """
    GET /api/device-settings/
    Returns the singleton device settings (poll_interval_sec, brightness, etc.).
    """
    def get(self, request):
        obj = DeviceSettings.objects.get_singleton()
        serializer = DeviceSettingsSerializer(obj)
        return Response(serializer.data)
