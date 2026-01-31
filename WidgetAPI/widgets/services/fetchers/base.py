from abc import ABC, abstractmethod


class BaseFetcher(ABC):
    """Abstract fetcher: takes widget configuration and returns data payload for display."""

    @abstractmethod
    def fetch(self, configuration: dict) -> dict:
        """
        Fetch real-time (or mock) data for the widget.

        :param configuration: WidgetInstance.configuration (e.g. {"city": "Ankara", "unit": "C"})
        :return: Data dict ready for API (e.g. {"temperature": 22, "condition": "Sunny"})
        """
        pass
