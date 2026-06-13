#include "layout.h"
#include "settings.h"
#include "../c/ataglance.h"

#define DESIGN_FONT_DATE_COMPACT FONT_KEY_GOTHIC_18_BOLD
#define DESIGN_FONT_TIME_COMPACT FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
#ifdef PBL_HEALTH
#define DESIGN_FONT_BPM_COMPACT FONT_KEY_GOTHIC_14
#define DESIGN_FONT_STEPS_COMPACT FONT_KEY_GOTHIC_14
#endif
#define DESIGN_FONT_BATTERY_COMPACT FONT_KEY_GOTHIC_14
#define DESIGN_FONT_CLIMATE_COMPACT FONT_KEY_GOTHIC_14

#define DESIGN_FONT_DATE_FULL FONT_KEY_GOTHIC_24_BOLD
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define DESIGN_FONT_TIME_FULL FONT_KEY_ROBOTO_BOLD_SUBSET_49
#else
#define DESIGN_FONT_TIME_FULL FONT_KEY_LECO_42_NUMBERS
#endif
#ifdef PBL_HEALTH
#define DESIGN_FONT_BPM_FULL FONT_KEY_GOTHIC_18
#define DESIGN_FONT_STEPS_FULL FONT_KEY_GOTHIC_18
#endif
#define DESIGN_FONT_BATTERY_FULL FONT_KEY_GOTHIC_18
#define DESIGN_FONT_CLIMATE_FULL FONT_KEY_GOTHIC_18

static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .background_layer_background = GColorBlack,
  .background_layer_rule = GColorWhite,
  .primary_text = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .background_layer_background = GColorWhite,
  .background_layer_rule = PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack),
  .primary_text = PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack),
  .date = GColorBlack,
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
};

static const char* layout_font_key_for_role(
    WatchfaceFontRole role,
    bool is_compact) {
  switch (role) {
    case WATCHFACE_FONT_ROLE_DATE:
      return is_compact ?
          DESIGN_FONT_DATE_COMPACT :
          DESIGN_FONT_DATE_FULL;
    case WATCHFACE_FONT_ROLE_TIME:
      return is_compact ?
          DESIGN_FONT_TIME_COMPACT :
          DESIGN_FONT_TIME_FULL;
#ifdef PBL_HEALTH
    case WATCHFACE_FONT_ROLE_BPM:
      return is_compact ?
          DESIGN_FONT_BPM_COMPACT :
          DESIGN_FONT_BPM_FULL;
    case WATCHFACE_FONT_ROLE_STEPS:
      return is_compact ?
          DESIGN_FONT_STEPS_COMPACT :
          DESIGN_FONT_STEPS_FULL;
#endif
    case WATCHFACE_FONT_ROLE_BATTERY:
      return is_compact ?
          DESIGN_FONT_BATTERY_COMPACT :
          DESIGN_FONT_BATTERY_FULL;
    case WATCHFACE_FONT_ROLE_CLIMATE:
      return is_compact ?
          DESIGN_FONT_CLIMATE_COMPACT :
          DESIGN_FONT_CLIMATE_FULL;
    case WATCHFACE_FONT_ROLE_COUNT:
      return DESIGN_FONT_CLIMATE_FULL;
    default:
      return DESIGN_FONT_CLIMATE_FULL;
  }
}

static uint32_t layout_custom_font_resource_id_for_role(
    WatchfaceFontRole role,
    bool is_compact) {
  if (role == WATCHFACE_FONT_ROLE_TIME && !is_compact) {
    return RESOURCE_ID_FONT_TIME_UNBOUNDED_48;
  }

  return 0;
}

static const ColorPalette* layout_palette_for_display_mode(uint8_t display_mode) {
  if (display_mode == DISPLAY_MODE_LIGHT) {
    return &c_light_palette;
  }

  return &c_dark_palette;
}

void layout_update_watchface_style(
    WatchfaceSurfaceStyle* style,
    uint8_t display_mode) {

  if (!style) {
    return;
  }

  style->palette = layout_palette_for_display_mode(display_mode);
  style->is_light_mode = (display_mode == DISPLAY_MODE_LIGHT);
  bool is_compact = style->is_compact;

  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    style->fonts[i] = fonts_get_system_font(
        layout_font_key_for_role((WatchfaceFontRole)i, is_compact));
    style->custom_font_resource_ids[i] =
        layout_custom_font_resource_id_for_role(
            (WatchfaceFontRole)i,
            is_compact);
  }
}
