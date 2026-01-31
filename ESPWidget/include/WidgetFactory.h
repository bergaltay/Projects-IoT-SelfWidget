/**
 * WidgetFactory - Build LVGL widgets from JSON (type + data) in a 2x2 grid.
 */
#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include <ArduinoJson.h>
#include "lvgl.h"

class WidgetFactory {
public:
    /**
     * Create a widget in the given grid cell (position_index 0-3) on parent.
     * type: e.g. "weather-card", "stock-card", "system-stats"
     * data: fetched data from API (temperature, price, etc.)
     * config: widget configuration (city, symbol, etc.)
     */
    static void createWidget(
        lv_obj_t *parent,
        int position_index,
        const char *type,
        JsonObject data,
        JsonObject config
    );
};

#endif /* WIDGET_FACTORY_H */
