"""
Beszel (System Monitor) fetcher via PocketBase REST API.
Uses standard HTTP requests - no pocketbase SDK.
"""
from typing import Optional

import requests

from .base import BaseFetcher

DEFAULT_ERROR = {'error': 'Connection failed'}
TIMEOUT = 10


class BeszelFetcher(BaseFetcher):
    """
    Fetches CPU, RAM, and temperature stats from Beszel via PocketBase REST API.
    Configuration: {"url": "...", "identity": "email", "password": "..."}
    """

    def fetch(self, configuration: dict) -> dict:
        url = (configuration.get('url') or '').rstrip('/')
        identity = configuration.get('identity') or configuration.get('email', '')
        password = configuration.get('password', '')

        if not url or not identity or not password:
            return {'error': 'Missing url, identity, or password in configuration'}

        try:
            token = self._auth(url, identity, password)
            if token is None:
                return DEFAULT_ERROR

            system_id = self._get_system_id(url, token)
            if system_id is None:
                return DEFAULT_ERROR

            stats = self._get_stats(url, token, system_id)
            if stats is None:
                return DEFAULT_ERROR

            return self._transform(stats)
        except (requests.RequestException, ValueError, KeyError, TypeError):
            return DEFAULT_ERROR

    def _auth(self, base_url: str, identity: str, password: str) -> Optional[str]:
        """Authenticate and return Bearer token."""
        resp = requests.post(
            f'{base_url}/api/collections/users/auth-with-password',
            json={'identity': identity, 'password': password},
            timeout=TIMEOUT,
        )
        resp.raise_for_status()
        data = resp.json()
        return data.get('token')

    def _get_system_id(self, base_url: str, token: str) -> Optional[str]:
        """Get the ID of the most recently updated system."""
        resp = requests.get(
            f'{base_url}/api/collections/systems/records',
            headers={'Authorization': f'Bearer {token}'},
            params={'sort': '-updated', 'perPage': 1},
            timeout=TIMEOUT,
        )
        resp.raise_for_status()
        data = resp.json()
        items = data.get('items') or []
        if not items:
            return None
        return items[0].get('id')

    def _get_stats(self, base_url: str, token: str, system_id: str) -> Optional[dict]:
        """Get system_stats for the given system."""
        resp = requests.get(
            f'{base_url}/api/collections/system_stats/records',
            headers={'Authorization': f'Bearer {token}'},
            params={
                'filter': f"system='{system_id}'",
                'sort': '-created',
                'perPage': 1,
            },
            timeout=TIMEOUT,
        )
        resp.raise_for_status()
        data = resp.json()
        items = data.get('items') or []
        if not items:
            return None
        return items[0].get('stats')

    def _transform(self, stats: dict) -> dict:
        """Transform stats to widget format: cpu, ram, temp."""
        cpu = stats.get('cpu')
        cpu_val = round(float(cpu), 1) if cpu is not None else 0.0

        ram = stats.get('mp')
        ram_val = round(float(ram), 1) if ram is not None else 0.0

        t = stats.get('t')
        temp_val = 0
        if t is not None:
            if isinstance(t, dict):
                values = list(t.values()) if t else []
            elif isinstance(t, (list, tuple)):
                values = list(t)
            else:
                values = [float(t)] if t != '' else []
            temp_val = int(round(max(values))) if values else 0

        return {
            'cpu': cpu_val,
            'ram': ram_val,
            'temp': temp_val,
        }
