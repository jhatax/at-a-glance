#include "climate.h"
#include "climate_glyphs.h"
#include "gcolor_definitions.h"
#include "helper.h"
#include "settings.h"
#include "substratum_renderer.h"

#define MAX_STR_LEN 16

enum {
  WEATHER_TEMP_MIN_CELSIUS_TENTHS = -1600,
  WEATHER_TEMP_MAX_CELSIUS_TENTHS = 1600,
  WEATHER_CONDITION_MIN = 0,
  WEATHER_CONDITION_MAX = 99,
};

// Initialize static variables to known starting state values
static Layer *s_climate_icon_layer = NULL;
static TextLayer *s_temperature_layer = NULL;
static char s_temperature_buffer[MAX_STR_LEN] = {0};
static int16_t s_temp_celsius_tenths = CLIMATE_TEMP_UNAVAILABLE;
static int16_t s_weather_condition = CLIMATE_CONDITION_UNKNOWN;
static bool s_is_day = false;

static ClimatePalette s_climate_palette = {0};

static bool s_climate_is_available = false;
static bool format_temperature(char *buf, size_t buflen, uint8_t temp_unit);
static bool climate_temperature_is_valid(int celsius_tenths);
static bool climate_condition_is_valid(int weather_condition);
static void climate_module_update_display(uint8_t temp_unit);
static void climate_module_set_temperature(int celsius_tenths);
static void climate_module_set_condition(int weather_condition);

static const ClimatePalette c_dark_climate_palette = {
    .sun = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
    .cold = PBL_IF_COLOR_ELSE(GColorMintGreen, GColorWhite),
    .cloud = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite),
    .clear_ring = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
    .clear_fill = GColorLightGray,
};

static const ClimatePalette c_light_climate_palette = {
    .sun = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorBlack),
    .cold = PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack),
    .cloud = PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack),
    .clear_ring = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack),
    .clear_fill = GColorLightGray,
};

static void climate_module_update_palette(const ColorPalette *palette) {
  const ClimatePalette *template =
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
  s_climate_palette.unknown = palette->unavailable_text;
}

static bool format_temperature(char *buf, size_t buflen, uint8_t temp_unit) {
  if (!buf || buflen == 0 || !MODULE_PALETTE_LOADED(s_climate_palette)) {
    return false;
  }

  if (s_temp_celsius_tenths == CLIMATE_TEMP_UNAVAILABLE) {
    const char *unit_text = HELPER_IF_ELSE((temp_unit == TEMP_UNIT_C), "C", "F");
    snprintf(buf, buflen, "%s%s", WATCHFACE_UNAVAILABLE_TEXT, unit_text);
    return false;
  }

  if (temp_unit == TEMP_UNIT_F) {
    int f_whole = ((s_temp_celsius_tenths * 9 + 25) / 50) + 32;
    snprintf(buf, buflen, "%dF", f_whole);
  } else {
    int c_whole = (s_temp_celsius_tenths >= 0) ? (s_temp_celsius_tenths + 5) / 10
                                               : (s_temp_celsius_tenths - 5) / 10;
    snprintf(buf, buflen, "%dC", c_whole);
  }

  return true;
}

static bool climate_temperature_is_valid(int celsius_tenths) {
  return celsius_tenths == CLIMATE_TEMP_UNAVAILABLE ||
         (celsius_tenths >= WEATHER_TEMP_MIN_CELSIUS_TENTHS &&
          celsius_tenths <= WEATHER_TEMP_MAX_CELSIUS_TENTHS);
}

static bool climate_condition_is_valid(int weather_condition) {
  return weather_condition == CLIMATE_CONDITION_UNKNOWN ||
         (weather_condition >= WEATHER_CONDITION_MIN && weather_condition <= WEATHER_CONDITION_MAX);
}

static bool climate_is_day_is_valid(int is_day) { return (is_day == 0 || is_day == 1); }

static void climate_module_update_display(uint8_t temp_unit) {
  if (!s_temperature_layer || !MODULE_PALETTE_LOADED(s_climate_palette)) {
    return;
  }

  bool is_temperature_available = format_temperature(s_temperature_buffer, MAX_STR_LEN, temp_unit);

  // Use formatting output to determine the color of text displayed
  GColor text_color =
      is_temperature_available ? s_climate_palette.normal : s_climate_palette.unknown;

  substratum_renderer_update_text_layer(s_temperature_layer, s_temperature_buffer, text_color);

  // Weather is available if there is a temperature and condition is known
  s_climate_is_available =
      is_temperature_available && (s_weather_condition != CLIMATE_CONDITION_UNKNOWN);

  if (s_climate_icon_layer) {
    layer_mark_dirty(s_climate_icon_layer);
  }
}

static void climate_icon_update_proc(Layer *layer, GContext *ctx) {
  if (!layer || !ctx || !MODULE_PALETTE_LOADED(s_climate_palette)) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_climate_palette.background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int16_t condition = s_weather_condition;

  if (!s_climate_is_available) {
    condition = CLIMATE_CONDITION_UNKNOWN;
  }

  draw_climate_icon(ctx, &bounds, condition, s_is_day, &s_climate_palette);
}

static void climate_module_set_temperature(int celsius_tenths) {
  if (!climate_temperature_is_valid(celsius_tenths)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather temperature invalid: value=%d", celsius_tenths);
    celsius_tenths = CLIMATE_TEMP_UNAVAILABLE;
  }

  s_temp_celsius_tenths = celsius_tenths;
}

static void climate_module_set_condition(int weather_condition) {
  if (!climate_condition_is_valid(weather_condition)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather condition invalid: value=%d", weather_condition);
    weather_condition = CLIMATE_CONDITION_UNKNOWN;
  }

  s_weather_condition = weather_condition;
}

// APIs called by other components of the watchface
bool climate_module_create(Layer *root, const WatchfaceTextSubstratum *text,
                           const WatchfaceIconSubstratum *icon, GFont font) {
  if (!root || !text || !font) {
    return false;
  }

  s_temperature_layer = substratum_renderer_create_text_layer(root, text, font);
  if (!s_temperature_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create climate temperature layer");
    return false;
  }

  if (icon) {
    s_climate_icon_layer =
        substratum_renderer_create_icon_layer(root, icon, climate_icon_update_proc);

    if (!s_climate_icon_layer) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create climate condition icon");
    }
  }

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

  s_temp_celsius_tenths = CLIMATE_TEMP_UNAVAILABLE;
  s_climate_is_available = false;
  s_is_day = false;
  s_weather_condition = CLIMATE_CONDITION_UNKNOWN;
  s_temperature_buffer[0] = '\0';
  s_climate_palette = (ClimatePalette){0};
}

void climate_module_refresh(const ColorPalette *palette, uint8_t temp_unit) {
  if (!palette) {
    return;
  }

  climate_module_update_palette(palette);
  climate_module_update_display(temp_unit);
}

void climate_module_set_weather(ClimateUpdate *update) {
  if (update == NULL) {
    return;
  }

  int celsius_tenths = update->celsius_tenths;
  int weather_condition = update->weather_condition;
  int is_day = update->is_day;
  if (!update->is_complete || !climate_temperature_is_valid(celsius_tenths) ||
      !climate_condition_is_valid(weather_condition) || !climate_is_day_is_valid(is_day)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather data invalid: temp=%d condition=%d is_day=%d",
            celsius_tenths, weather_condition, is_day);
    // Invalidate previous weather data
    s_temp_celsius_tenths = CLIMATE_TEMP_UNAVAILABLE;
    s_weather_condition = CLIMATE_CONDITION_UNKNOWN;
    s_is_day = false;
  } else {
    climate_module_set_temperature(celsius_tenths);
    climate_module_set_condition(weather_condition);
    // Is it really the day or is it night?
    s_is_day = (is_day == 1);
  }
}
