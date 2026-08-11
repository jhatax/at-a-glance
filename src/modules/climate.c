#include "climate.h"

#include <ctype.h>
#include <stdint.h>

#include "climate_glyphs.h"
#include "helper.h"
#include "settings.h"
#include "substratum_renderer.h"

#define MAX_TEMPERATURE_STR_LEN 16

enum {
  CLIMATE_LOCATION_MAX_CHARACTERS = 15,
  CLIMATE_LOCATION_MAX_UTF8_BYTES = CLIMATE_LOCATION_MAX_CHARACTERS * 2,
  CLIMATE_LOCATION_BUFFER_SIZE = CLIMATE_LOCATION_MAX_UTF8_BYTES + 1,
};

enum {
  WEATHER_TEMP_MIN_CELSIUS_TENTHS = -1600,
  WEATHER_TEMP_MAX_CELSIUS_TENTHS = 1600,
  WEATHER_TEMP_OUTOFRANGE = WEATHER_TEMP_MIN_CELSIUS_TENTHS - 1,
};

#define TEMPERATURE_IS_VALID(cel_tenths) \
  (HELPER_VALUE_IN_RANGE((cel_tenths),   \
      WEATHER_TEMP_MIN_CELSIUS_TENTHS,   \
      WEATHER_TEMP_MAX_CELSIUS_TENTHS))

#define VALID_IS_DAY(s) HELPER_VALUE_IN_RANGE((s), 0, 1)

// Initialize static variables to known starting state values
static Layer* s_climate_icon_layer = NULL;
static TextLayer* s_temperature_layer = NULL;
static TextLayer* s_location_layer = NULL;
static char s_temperature_buffer[MAX_TEMPERATURE_STR_LEN] = {0};
static char s_location_buffer[CLIMATE_LOCATION_BUFFER_SIZE] = {0};
static int16_t s_temp_celsius_tenths = WEATHER_TEMP_OUTOFRANGE;
static int16_t s_weather_condition = CLIMATE_CONDITION_OUTOFRANGE;
static bool s_is_day = false;

static ClimatePalette s_climate_palette = {0};

static bool s_climate_is_available = false;
static bool format_temperature(char* buf,
    size_t buflen,
    uint8_t temp_unit);
static void climate_module_update_temperature(uint8_t temp_unit);
static void climate_module_set_temperature(int celsius_tenths);
static void climate_module_set_condition(int weather_condition);

static const ClimatePalette c_dark_climate_palette = {
    .sun = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
    .cold = GColorWhite,
    .cloud = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite),
    .clear_ring = PBL_IF_COLOR_ELSE(GColorBabyBlueEyes, GColorLightGray),
    .clear_fill = PBL_IF_COLOR_ELSE(GColorBabyBlueEyes, GColorDarkGray),
};

static const ClimatePalette c_light_climate_palette = {
    .sun = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorBlack),
    .cold = GColorDarkGray,
    .cloud = PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack),
    .clear_ring = GColorDarkGray,
    .clear_fill = GColorLightGray,
};

static void climate_module_update_palette(
    const ColorPalette* palette) {
  const ClimatePalette* template =
      palette->is_light_mode ? &c_light_climate_palette : &c_dark_climate_palette;

  if (MODULE_PALETTE_LOADED(s_climate_palette)) {
    if (HELPER_COLOR_EQUAL(s_climate_palette.background, palette->background)) {
      // The palette doesn't need to be updated
      return;
    }
  }
  // Two possibilities:
  // 1. First time the palette is being initialized
  // 2. The palette has changed
  s_climate_palette = *template;
  s_climate_palette.background = palette->background;
  s_climate_palette.normal = palette->primary_text;
  s_climate_palette.outofrange = palette->outofrange_text;
}

static bool format_temperature(
    char* buf,
    size_t buflen,
    uint8_t temp_unit) {
  if (!buf || buflen == 0 || !MODULE_PALETTE_LOADED(s_climate_palette)) {
    return false;
  }

  if (!(TEMPERATURE_IS_VALID(s_temp_celsius_tenths))) {
    const char* unit_text = HELPER_IF_ELSE((temp_unit == TEMP_UNIT_C), "C", "F");
    snprintf(buf, buflen, "%s%s", WATCHFACE_OUTOFRANGE_TEXT, unit_text);
    return false;
  }

  if (temp_unit == TEMP_UNIT_F) {
    int f_whole = HELPER_ROUND_UP((s_temp_celsius_tenths * 9 + 1600), 50);
    snprintf(buf, buflen, "%dF", f_whole);
  } else {
    int c_whole = HELPER_ROUND_UP(s_temp_celsius_tenths, 10);
    snprintf(buf, buflen, "%dC", c_whole);
  }

  return true;
}

static void climate_module_update_temperature(
    uint8_t temp_unit) {
  if (!s_temperature_layer || !MODULE_PALETTE_LOADED(s_climate_palette)) {
    return;
  }

  bool is_temperature_available =
      format_temperature(s_temperature_buffer, MAX_TEMPERATURE_STR_LEN, temp_unit);

  // Use formatting output to determine the color of text displayed
  GColor text_color =
      is_temperature_available ? s_climate_palette.normal : s_climate_palette.outofrange;

  substratum_renderer_update_text_layer(s_temperature_layer, s_temperature_buffer, text_color);

  // Weather is available if there is a temperature and condition is known
  s_climate_is_available =
      is_temperature_available && (s_weather_condition != CLIMATE_CONDITION_OUTOFRANGE);

  if (s_climate_icon_layer) {
    layer_mark_dirty(s_climate_icon_layer);
  }
}

static void climate_module_update_location() {
  if (!s_location_layer || !MODULE_PALETTE_LOADED(s_climate_palette)) {
    return;
  }

  substratum_renderer_update_text_layer(s_location_layer,
      s_location_buffer,
      (0 == strlen(s_location_buffer)) ? s_climate_palette.outofrange : s_climate_palette.normal);
}

static void climate_icon_update_proc(
    Layer* layer,
    GContext* ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_climate_palette)) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_climate_palette.background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int16_t condition = s_weather_condition;

  if (!s_climate_is_available) {
    condition = CLIMATE_CONDITION_OUTOFRANGE;
  }

  draw_climate_icon(ctx, &bounds, condition, s_is_day, &s_climate_palette);
}

static void climate_module_set_temperature(
    int celsius_tenths) {
  if (!(TEMPERATURE_IS_VALID(celsius_tenths))) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather temperature invalid: value=%d", celsius_tenths);
    celsius_tenths = WEATHER_TEMP_OUTOFRANGE;
  }

  s_temp_celsius_tenths = celsius_tenths;
}

static void climate_module_set_condition(
    int weather_condition) {
  if (!(CLIMATE_CONDITION_IS_VALID(weather_condition))) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather condition invalid: value=%d", weather_condition);
    weather_condition = CLIMATE_CONDITION_OUTOFRANGE;
  }

  s_weather_condition = weather_condition;
}

// APIs called by other components of the watchface
bool climate_module_create(
    Layer* root,
    const WatchfaceTextSubstratum* temp,
    const WatchfaceTextSubstratum* loc,
    const WatchfaceIconSubstratum* condition,
    GFont text_font,
    GFont location_font) {
  if (!root || !temp || !text_font || !location_font) {
    return false;
  }

  s_temperature_layer = substratum_renderer_create_text_layer(root, temp, text_font);
  if (!s_temperature_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create climate temperature layer");
    return false;
  }

  s_location_layer = substratum_renderer_create_text_layer(root, loc, location_font);
  if (!s_location_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create climate location layer");
    return false;
  }

  if (condition) {
    s_climate_icon_layer =
        substratum_renderer_create_icon_layer(root, condition, climate_icon_update_proc);

    if (!s_climate_icon_layer) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create climate condition icon");
    }
  }

  return true;
}

void climate_module_destroy() {
  if (s_climate_icon_layer) {
    layer_destroy(s_climate_icon_layer);
    s_climate_icon_layer = NULL;
  }
  if (s_temperature_layer) {
    text_layer_destroy(s_temperature_layer);
    s_temperature_layer = NULL;
  }
  if (s_location_layer) {
    text_layer_destroy(s_location_layer);
    s_location_layer = NULL;
  }

  s_temp_celsius_tenths = WEATHER_TEMP_OUTOFRANGE;
  s_climate_is_available = false;
  s_is_day = false;
  s_weather_condition = CLIMATE_CONDITION_OUTOFRANGE;
  memset(s_temperature_buffer, 0, sizeof(s_temperature_buffer));
  memset(s_location_buffer, 0, sizeof(s_location_buffer));
  s_climate_palette = (ClimatePalette){0};
}

void climate_module_refresh(
    const ColorPalette* palette,
    const WatchfaceUpdateMask refreshed,
    uint8_t temp_unit) {
  if (!palette) {
    return;
  }

  climate_module_update_palette(palette);
  if (refreshed == WATCHFACE_UPDATE_CLIMATE) {
    climate_module_update_temperature(temp_unit);
  } else if (refreshed == WATCHFACE_UPDATE_LOCATION) {
    climate_module_update_location();
  }
}

void climate_module_set_weather(
    ClimateUpdate* update) {
  if (update == NULL) {
    return;
  }

  int celsius_tenths = update->celsius_tenths;
  int weather_condition = update->weather_condition;
  int is_day = update->is_day;
  if (!update->is_complete || !(TEMPERATURE_IS_VALID(celsius_tenths)) ||
      !(CLIMATE_CONDITION_IS_VALID(weather_condition)) || !(VALID_IS_DAY(is_day))) {
    APP_LOG(APP_LOG_LEVEL_WARNING,
        "Weather data invalid: temp=%d condition=%d is_day=%d",
        celsius_tenths,
        weather_condition,
        is_day);
    // Invalidate previous weather data
    s_temp_celsius_tenths = WEATHER_TEMP_OUTOFRANGE;
    s_weather_condition = CLIMATE_CONDITION_OUTOFRANGE;
    s_is_day = false;
  } else {
    climate_module_set_temperature(celsius_tenths);
    climate_module_set_condition(weather_condition);
    // Is it really the day or is it night?
    s_is_day = (is_day == 1);
  }
}

void climate_module_set_location(
    char* location) {
  if (location == NULL) {
    return;
  }
  snprintf(s_location_buffer, ARRAY_LENGTH(s_location_buffer), "%s", location);
}
