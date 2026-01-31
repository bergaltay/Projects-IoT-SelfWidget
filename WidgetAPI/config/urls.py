"""
URL configuration for WidgetAPI.
"""
from django.contrib import admin
from django.urls import path, include
from django.views.generic import RedirectView

urlpatterns = [
    path('', RedirectView.as_view(url='/api/screens/', permanent=False)),
    path('admin/', admin.site.urls),
    path('api/', include('widgets.urls')),
]
