#include "watchface_composer.h"
#include "battery.h"
#include "date.h"
#include "layout.h"
#include "time.h"
#include "weather.h"

#if defined(PBL_HEALTH)
#include "health.h"
#endif

static WatchfaceLayout s_layout;
static GFont s_secondary_value_font;
static GFont s_battery_value_font;

void watchface_composer_init(void) {
  s_secondary_value_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_battery_value_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  weather_module_init();
}

bool watchface_composer_create(
    Window* window,
    const WatchfaceSettings* settings,
    const VisualPalette* palette) {
  if (!window || !settings) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot create watchface without composer inputs");
    return false;
  }

  const VisualPalette* resolved_palette =
      display_resolve_palette(palette, settings->display_mode);

  Layer* root = window_get_root_layer(window);
  if (!root) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window root layer is NULL");
    return false;
  }

  GRect bounds = layer_get_bounds(root);
  layout_calculate(bounds.size.w, bounds.size.h, &s_layout);

  bool date_created = date_module_create(
      root,
      &s_layout.date_frame,
      resolved_palette);
  bool time_created = time_module_create(
      root,
      &s_layout.time_frame,
      resolved_palette);

  #if defined(PBL_HEALTH)
  bool health_created = health_module_create(
      root,
      &s_layout,
      s_secondary_value_font,
      resolved_palette);
  #else
  bool health_created = true;
  #endif

  bool weather_created = weather_module_create(
      root,
      &s_layout,
      settings->temp_unit,
      resolved_palette);
  bool battery_created = battery_module_create(
      root,
      &s_layout,
      s_battery_value_font,
      resolved_palette);

  bool created = date_created &&
      time_created &&
      health_created &&
      weather_created &&
      battery_created;

  if (!created) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Watchface must-initialize controls failed");
    watchface_composer_destroy();
    return false;
  }

  return created;
}

void watchface_composer_destroy(void) {
  date_module_destroy();
  time_module_destroy();

  #if defined(PBL_HEALTH)
  health_module_destroy();
  #endif

  weather_module_destroy();
  battery_module_destroy();
}

void watchface_composer_refresh(
    Window* window,
    const WatchfaceSettings* settings,
    const VisualPalette* palette) {
  if (!window || !settings) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot refresh watchface without composer inputs");
    return;
  }

  const VisualPalette* resolved_palette =
      display_resolve_palette(palette, settings->display_mode);

  window_set_background_color(window, resolved_palette->background);

  #if defined(PBL_HEALTH)
  health_module_refresh(resolved_palette);
  #endif

  date_module_refresh(resolved_palette);
  time_module_refresh(settings->time_format, resolved_palette);
  battery_module_refresh(resolved_palette);
  weather_module_refresh(settings->temp_unit, resolved_palette);
}

void watchface_composer_handle_tick(
    TimeUnits units_changed,
    const WatchfaceSettings* settings,
    const VisualPalette* palette) {
  if (!settings) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot refresh tick display without composer inputs");
    return;
  }

  const VisualPalette* resolved_palette =
      display_resolve_palette(palette, settings->display_mode);

  if (units_changed & MINUTE_UNIT) {
    time_module_refresh(settings->time_format, resolved_palette);
  }
  if (units_changed & DAY_UNIT) {
    date_module_refresh(resolved_palette);
  }
}
