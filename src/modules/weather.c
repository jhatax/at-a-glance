#include "weather.h"
#include "helper.h"
#include "layout.h"
#include "settings.h"
#include "weather_glyphs.h"
#include "../c/ataglance.h"

#define WEATHER_TEMP_MIN_CELSIUS_TENTHS -1600
#define WEATHER_TEMP_MAX_CELSIUS_TENTHS 1000
#define WEATHER_CONDITION_MIN 0
#define WEATHER_CONDITION_MAX 99

// Initialize static variables to known starting state values
static Layer* s_weather_icon_layer = NULL;
static TextLayer* s_temperature_layer = NULL;
static char s_temperature_buffer[ATAGLANCE_MAX_STR_LEN];
static int16_t s_temp_celsius_tenths = WEATHER_TEMP_INVALID;
static int16_t s_weather_condition = WEATHER_CONDITION_UNKNOWN;
static const VisualPalette* s_weather_palette = NULL;
static bool s_weather_condition_known = false;

static bool format_temperature(char* buf, size_t buflen, uint8_t temp_unit);
static inline bool weather_temperature_is_valid(int celsius_tenths);
static inline bool weather_condition_is_valid(int weather_condition);
static void weather_module_update_display(uint8_t temp_unit);

static bool format_temperature(char* buf, size_t buflen, uint8_t temp_unit) {
  if (!buf || buflen == 0) {
    return false;
  }

  if (s_temp_celsius_tenths == WEATHER_TEMP_INVALID) {
    const char* unit_text =
        temp_unit == TEMP_UNIT_C ? "°C" : "°F";
    snprintf(buf, buflen, "%s%s", DISPLAY_UNAVAILABLE_TEXT, unit_text);
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

static inline bool weather_temperature_is_valid(int celsius_tenths) {
  return celsius_tenths == WEATHER_TEMP_INVALID ||
      (
          celsius_tenths >= WEATHER_TEMP_MIN_CELSIUS_TENTHS &&
          celsius_tenths <= WEATHER_TEMP_MAX_CELSIUS_TENTHS
      );
}

static inline bool weather_condition_is_valid(int weather_condition) {
  return weather_condition == WEATHER_CONDITION_UNKNOWN ||
      (
          weather_condition >= WEATHER_CONDITION_MIN &&
          weather_condition <= WEATHER_CONDITION_MAX
      );
}

static void weather_module_update_display(uint8_t temp_unit) {
  if (!s_temperature_layer || !s_weather_palette) {
    return;
  }

  bool is_temperature_available = format_temperature(
      s_temperature_buffer,
      ATAGLANCE_MAX_STR_LEN,
      temp_unit);
  GColor text_color = is_temperature_available ?
      s_weather_palette->primary_text :
      s_weather_palette->unavailable_text;

  display_update_text_layer(
      s_temperature_layer,
      s_temperature_buffer,
      text_color);

  // Weather is available if there is a temperature and condition is known
  s_weather_condition_known =
      is_temperature_available &&
      (s_weather_condition != WEATHER_CONDITION_UNKNOWN);

  if (s_weather_icon_layer) {
    layer_mark_dirty(s_weather_icon_layer);
  }
}

static void weather_icon_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_weather_palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_weather_palette->background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int16_t _condition = s_weather_condition;

  if (!s_weather_condition_known) {
    _condition = WEATHER_CONDITION_UNKNOWN;
  }

  draw_weather_icon(ctx, &bounds, _condition, s_weather_palette);
}

// APIs called by other components of the watchface
bool weather_module_create(
    Layer* root,
    const WatchfaceLayout* layout,
    uint8_t temp_unit,
    const VisualPalette* palette) {
  if (!root || !layout || !palette) {
    return false;
  }

  s_weather_palette = palette;
  s_temperature_layer = text_layer_create(layout->temp_text_frame);
  if (!s_temperature_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create weather temperature layer");
    return false;
  }

  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_font(
      s_temperature_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(
      s_temperature_layer,
      GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_temperature_layer));

  s_weather_icon_layer = layer_create(layout->weather_icon_frame);
  if (s_weather_icon_layer) {
    layer_set_update_proc(
        s_weather_icon_layer,
        weather_icon_update_proc);
    layer_add_child(root, s_weather_icon_layer);
  }

  // Update display now that this module has been created
  weather_module_update_display(temp_unit);
  return true;
}

void weather_module_destroy(void) {
  if (s_weather_icon_layer) {
    layer_destroy(s_weather_icon_layer);
    s_weather_icon_layer = NULL;
  }
  if (s_temperature_layer) {
    text_layer_destroy(s_temperature_layer);
    s_temperature_layer = NULL;
  }

  s_temperature_buffer[0] = '\0';
}

void weather_module_refresh(uint8_t temp_unit, const VisualPalette* palette) {
  if (palette) {
    s_weather_palette = palette;
  }

  weather_module_update_display(temp_unit);
}

void weather_module_set_temperature(
    int celsius_tenths,
    uint8_t temp_unit,
    const VisualPalette* palette) {
  if (!weather_temperature_is_valid(celsius_tenths)) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Weather temperature invalid: value=%d",
            celsius_tenths);
    celsius_tenths = WEATHER_TEMP_INVALID;
  }

  s_temp_celsius_tenths = celsius_tenths;
  weather_module_refresh(temp_unit, palette);
}

void weather_module_set_condition(
    int weather_condition,
    uint8_t temp_unit,
    const VisualPalette* palette) {
  if (!weather_condition_is_valid(weather_condition)) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
            "Weather condition invalid: value=%d",
            weather_condition);
    weather_condition = WEATHER_CONDITION_UNKNOWN;
  }

  s_weather_condition = weather_condition;
  weather_module_refresh(temp_unit, palette);
}
