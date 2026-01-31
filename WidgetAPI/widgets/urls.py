from django.urls import path
from . import views

app_name = 'widgets'
urlpatterns = [
    path('screens/', views.ScreenListView.as_view(), name='screen-list'),
    path('device-settings/', views.DeviceSettingsView.as_view(), name='device-settings'),
]
