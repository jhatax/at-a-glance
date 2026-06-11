#include "layout_stylist.h"
#include "settings.h"
#include "../c/ataglance.h"

static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .background_layer_background = GColorBlack,
  .background_layer_line = GColorWhite,
  .primary_text = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .background_layer_background = GColorWhite,
  .background_layer_line = PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack),
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
          ATAGLANCE_FONT_KEY_DATE_COMPACT :
          ATAGLANCE_FONT_KEY_DATE_FULL;
    case WATCHFACE_FONT_ROLE_TIME:
      return is_compact ?
          ATAGLANCE_FONT_KEY_TIME_COMPACT :
          ATAGLANCE_FONT_KEY_TIME_FULL;
    case WATCHFACE_FONT_ROLE_BPM:
      return is_compact ?
          ATAGLANCE_FONT_KEY_BPM_COMPACT :
          ATAGLANCE_FONT_KEY_BPM_FULL;
    case WATCHFACE_FONT_ROLE_STEPS:
      return is_compact ?
          ATAGLANCE_FONT_KEY_STEPS_COMPACT :
          ATAGLANCE_FONT_KEY_STEPS_FULL;
    case WATCHFACE_FONT_ROLE_BATTERY:
      return is_compact ?
          ATAGLANCE_FONT_KEY_BATTERY_COMPACT :
          ATAGLANCE_FONT_KEY_BATTERY_FULL;
    case WATCHFACE_FONT_ROLE_CLIMATE:
      return is_compact ?
          ATAGLANCE_FONT_KEY_CLIMATE_COMPACT :
          ATAGLANCE_FONT_KEY_CLIMATE_FULL;
    case WATCHFACE_FONT_ROLE_COUNT:
      return ATAGLANCE_FONT_KEY_BPM_FULL;
    default:
      return ATAGLANCE_FONT_KEY_BPM_FULL;
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

void layout_stylist_update_surface_style(
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
