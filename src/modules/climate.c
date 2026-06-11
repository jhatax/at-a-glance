#include "climate.h"
#include "climate_glyphs.h"
#include "settings.h"
#include "substratum_renderer.h"
#include "../c/ataglance.h"

#define WEATHER_TEMP_MIN_CELSIUS_TENTHS -1600
#define WEATHER_TEMP_MAX_CELSIUS_TENTHS 1000
#define WEATHER_CONDITION_MIN 0
#define WEATHER_CONDITION_MAX 99

// Initialize static variables to known starting state values
static Layer* s_climate_icon_layer = NULL;
static TextLayer* s_temperature_layer = NULL;
static char s_temperature_buffer[ATAGLANCE_MAX_STR_LEN] = {0};
static int16_t s_temp_celsius_tenths = WEATHER_TEMP_INVALID;
static int16_t s_weather_condition = WEATHER_CONDITION_UNKNOWN;
static const WatchfaceSurface* s_surface = NULL;
static bool s_weather_condition_known = false;

static bool format_temperature(char* buf, size_t buflen, uint8_t temp_unit);
static bool climate_temperature_is_valid(int celsius_tenths);
static bool climate_condition_is_valid(int weather_condition);
static void climate_module_update_display(uint8_t temp_unit);

static bool format_temperature(char* buf, size_t buflen, uint8_t temp_unit) {
  if (!buf || buflen == 0) {
    return false;
  }

  if (s_temp_celsius_tenths == WEATHER_TEMP_INVALID) {
    const char* unit_text =
        temp_unit == TEMP_UNIT_C ? "°C" : "°F";
    snprintf(buf, buflen, "%s%s", WATCHFACE_UNAVAILABLE_TEXT, unit_text);
    return false;
  }

  if (temp_unit == TEMP_UNIT_F) {
    int f_whole = ((s_temp_celsius_tenths * 9 + 25) / 50) + 32;
    snprintf(buf, buflen, "%d°F", f_whole);
  } else {
    int c_whole = (s_temp_celsius_tenths >= 0) ?
        (s_temp_celsius_tenths + 5) / 10 :
        (s_temp_celsius_tenths - 5) / 10;
    snprintf(buf, buflen, "%d°C", c_whole);
  }

  return true;
}

static bool climate_temperature_is_valid(int celsius_tenths) {
  return celsius_tenths == WEATHER_TEMP_INVALID ||
      (
          celsius_tenths >= WEATHER_TEMP_MIN_CELSIUS_TENTHS &&
          celsius_tenths <= WEATHER_TEMP_MAX_CELSIUS_TENTHS
      );
}

static bool climate_condition_is_valid(int weather_condition) {
  return weather_condition == WEATHER_CONDITION_UNKNOWN ||
      (
          weather_condition >= WEATHER_CONDITION_MIN &&
          weather_condition <= WEATHER_CONDITION_MAX
      );
}

static void climate_module_update_display(uint8_t temp_unit) {
  if (!s_temperature_layer || !s_surface || !s_surface->style.palette) {
    return;
  }

  const ColorPalette* palette = s_surface->style.palette;
  bool is_temperature_available = format_temperature(
      s_temperature_buffer,
      ATAGLANCE_MAX_STR_LEN,
      temp_unit);
  GColor text_color = is_temperature_available ?
      palette->primary_text :
      palette->unavailable_text;

  substratum_renderer_update_text_layer(
      s_temperature_layer,
      s_temperature_buffer,
      text_color);

  // Weather is available if there is a temperature and condition is known
  s_weather_condition_known =
      is_temperature_available &&
      (s_weather_condition != WEATHER_CONDITION_UNKNOWN);

  if (s_climate_icon_layer) {
    layer_mark_dirty(s_climate_icon_layer);
  }
}

static void climate_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface || !s_surface->style.palette) {
    return;
  }

  const ColorPalette* palette = s_surface->style.palette;
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int16_t _condition = s_weather_condition;

  if (!s_weather_condition_known) {
    _condition = WEATHER_CONDITION_UNKNOWN;
  }

  draw_climate_icon(ctx, &bounds, _condition, palette);
}

// APIs called by other components of the watchface
bool climate_module_create(
    Layer* root,
    const WatchfaceSurface* surface,
    uint8_t temp_unit) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  const WatchfaceTextSubstratum* text = &surface->climate.text;
  const WatchfaceIconSubstratum* icon = &surface->climate.icon;
  s_temperature_layer = substratum_renderer_create_text_layer(
      root,
      text,
      surface->style.fonts[text->font_role]);
  if (!s_temperature_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create climate temperature layer");
    return false;
  }

  s_surface = surface;
  s_climate_icon_layer = substratum_renderer_create_icon_layer(
      root,
      icon,
      climate_icon_update_proc);

  // Update display now that this module has been created
  climate_module_update_display(temp_unit);
  return true;
}

void climate_module_destroy(void) {
  if (s_climate_icon_layer) {
    layer_destroy(s_climate_icon_layer);
    s_climate_icon_layer = NULL;
  }
  if (s_temperature_layer) {
    text_layer_destroy(s_temperature_layer);
    s_temperature_layer = NULL;
  }

  s_temperature_buffer[0] = '\0';
  s_surface = NULL;
}

void climate_module_refresh(uint8_t temp_unit) {
  climate_module_update_display(temp_unit);
}

void climate_module_set_temperature(int celsius_tenths) {
  if (!climate_temperature_is_valid(celsius_tenths)) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Weather temperature invalid: value=%d",
            celsius_tenths);
    celsius_tenths = WEATHER_TEMP_INVALID;
  }

  s_temp_celsius_tenths = celsius_tenths;
}

void climate_module_set_condition(int weather_condition) {
  if (!climate_condition_is_valid(weather_condition)) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Weather condition invalid: value=%d",
            weather_condition);
    weather_condition = WEATHER_CONDITION_UNKNOWN;
  }

  s_weather_condition = weather_condition;
}
