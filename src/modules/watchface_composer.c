#include "watchface_composer.h"
#include "battery.h"
#include "date.h"
#include "layout.h"
#include "time.h"
#include "weather.h"

#ifdef PBL_HEALTH
#include "health.h"
#endif

static WatchfaceLayout s_layout = {0};
static Window* s_wf_window = NULL;
static const WatchfaceSettings* s_wf_settings = NULL;

typedef enum {
  NO_LAYERS_MASK = 0,
  DATE_LAYER_MASK = 1,
  TIME_LAYER_MASK = 2,
  BATTERY_LAYER_MASK = 4,
  MUST_HAVE_LAYERS_MASK = 7,
  WEATHER_LAYER_MASK = 8,
#ifdef PBL_HEALTH
  HEALTH_LAYER_MASK = 16,
  ALL_LAYERS_MASK = 31
#else
  ALL_LAYERS_MASK = 15
#endif
} LayerMask;
static uint8_t s_layers_created = (uint8_t) NO_LAYERS_MASK;

static void watchface_composer_exit_path() {
  watchface_composer_destroy();
  window_stack_pop_all(false);
}

bool watchface_composer_create(
    Window* window,
    const WatchfaceSettings* settings) {
  if (!window || !settings) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot create watchface without composer inputs");
    if (window) {
      watchface_composer_exit_path();
    }
    return false;
  }

  s_wf_window = window;
  s_wf_settings = settings;

  Layer* root = window_get_root_layer(window);

  if (!root) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window root layer is NULL");
    watchface_composer_exit_path();
    return false;
  }

  // API contract is a visual palette is always returned
  const VisualPalette* _palette = display_get_palette(s_wf_settings->display_mode);

  // No layers created
  s_layers_created = (uint8_t) 0;

  GRect bounds = layer_get_bounds(root);
  layout_calculate(bounds.size.w, bounds.size.h, &s_layout);

  s_layers_created |= date_module_create(root, &s_layout.date_frame, _palette)
                    ? DATE_LAYER_MASK : 0;

  s_layers_created |= time_module_create(root, &s_layout.time_frame, _palette)
                    ? TIME_LAYER_MASK : 0;

  s_layers_created |= battery_module_create(root, &s_layout, _palette)
                    ? BATTERY_LAYER_MASK : 0;

  // Enforce the Strict Core Contract Validation Check
  if ((s_layers_created & MUST_HAVE_LAYERS_MASK) != MUST_HAVE_LAYERS_MASK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface must-initialize controls failed");
    watchface_composer_exit_path();
    return false;
  }

  // Proceed with optional layers
  s_layers_created |= weather_module_create(root, &s_layout, s_wf_settings->temp_unit, _palette)
                    ? WEATHER_LAYER_MASK : 0;

  #ifdef PBL_HEALTH
  s_layers_created |= health_module_create(root, &s_layout, _palette)
                    ? HEALTH_LAYER_MASK : 0;
  #endif

  // Must-have layer creation succeeded, we can return TRUE
  return true;
}

void watchface_composer_destroy() {
  if (s_layers_created) {
    // Date
    if (s_layers_created & DATE_LAYER_MASK) {
      date_module_destroy();
      s_layers_created &= ~DATE_LAYER_MASK; // Clear the bit
    }

    // Time
    if (s_layers_created & TIME_LAYER_MASK) {
      time_module_destroy();
      s_layers_created &= ~TIME_LAYER_MASK;
    }

    // Battery
    if (s_layers_created & BATTERY_LAYER_MASK) {
      battery_module_destroy();
      s_layers_created &= ~BATTERY_LAYER_MASK;
    }

    // Weather
    if (s_layers_created & WEATHER_LAYER_MASK) {
      weather_module_destroy();
      s_layers_created &= ~WEATHER_LAYER_MASK;
    }

    #ifdef PBL_HEALTH
    // Health
    if (s_layers_created & HEALTH_LAYER_MASK) {
      health_module_destroy();
      s_layers_created &= ~HEALTH_LAYER_MASK;
    }
    #endif

  }
  // Reset all (async callbacks can still fire)
  s_layers_created = (uint8_t) NO_LAYERS_MASK;
  s_wf_settings = NULL;
  s_wf_window = NULL;
  memset(&s_layout, 0, sizeof(s_layout));
}

void watchface_composer_refresh() {
  if (!s_wf_window || !s_wf_settings) {
    return;
  }

  const VisualPalette* _palette = display_get_palette(s_wf_settings->display_mode);

  window_set_background_color(s_wf_window, _palette->background);

  // If we are here, then the mandatory modules were initialized
  date_module_refresh(_palette);
  time_module_refresh(s_wf_settings->time_format, _palette);
  battery_module_refresh(_palette);

  if (s_layers_created & WEATHER_LAYER_MASK) {
    weather_module_refresh(s_wf_settings->temp_unit, _palette);
  }

  #ifdef PBL_HEALTH
  if (s_layers_created & HEALTH_LAYER_MASK) {
    health_module_refresh(_palette);
  }
  #endif
}

void watchface_composer_handle_tick(TimeUnits units_changed) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  const VisualPalette* _palette = display_get_palette(s_wf_settings->display_mode);

  if (units_changed & MINUTE_UNIT) {
    time_module_refresh(s_wf_settings->time_format, _palette);
  }
  if (units_changed & DAY_UNIT) {
    date_module_refresh(_palette);
  }
}

void watchface_composer_update_temp(int celsius_tenths, uint8_t temp_unit) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  weather_module_set_temperature(
    celsius_tenths,
    temp_unit,
    display_get_palette(s_wf_settings->display_mode));
}

void watchface_composer_update_weather_condition(int weather_condition, uint8_t temp_unit) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  weather_module_set_condition(
    weather_condition,
    temp_unit,
    display_get_palette(s_wf_settings->display_mode));
}
