from rest_framework import generics
from rest_framework.views import APIView
from rest_framework.response import Response
from .models import Screen, DeviceSettings, ActiveNotification
from .serializers import ScreenSerializer, DeviceSettingsSerializer, ActiveNotificationSerializer


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


class CurrentNotificationView(APIView):
    """
    GET /api/notifications/current/
    Returns the single highest-priority active notification for the device.
    If no active notification: 200 with { "active": false } so the client always gets JSON.
    """
    def get(self, request):
        notification = (
            ActiveNotification.objects.filter(is_active=True)
            .order_by('-created_at')
            .first()
        )
        if notification is None:
            return Response({'active': False})
        serializer = ActiveNotificationSerializer(notification)
        data = dict(serializer.data)
        data['active'] = True
        return Response(data)
