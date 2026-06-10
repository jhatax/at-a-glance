#include "layout_stylist.h"
#include "settings.h"
#include "../c/ataglance.h"

static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .background_layer_background = GColorBlack,
  .background_layer_line = GColorWhite,
  .primary_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorRichBrilliantLavender, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .background_layer_background = GColorWhite,
  .background_layer_line = GColorBlack,
  .primary_text = GColorBlack,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorImperialPurple, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorBlack),
};

static bool layout_is_compact_display(
    int16_t face_width,
    int16_t face_height) {
#if defined(PBL_ROUND)
  return face_width < ATAGLANCE_ROUND_FULL_FACE_WIDTH ||
      face_height < ATAGLANCE_ROUND_FULL_FACE_HEIGHT;
#else
  return face_width < ATAGLANCE_DESIGN_FACE_WIDTH ||
      face_height < ATAGLANCE_DESIGN_FACE_HEIGHT;
#endif
}

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

static const ColorPalette* layout_palette_for_display_mode(
    uint8_t display_mode) {
  if (display_mode == DISPLAY_MODE_LIGHT) {
    return &c_light_palette;
  }

  return &c_dark_palette;
}

void layout_stylist_update_surface_style(
    int16_t face_width,
    int16_t face_height,
    uint8_t display_mode,
    WatchfaceSurfaceStyle* style) {
  if (!style) {
    return;
  }

  const bool is_compact = layout_is_compact_display(
      face_width,
      face_height);
  style->palette = layout_palette_for_display_mode(display_mode);

  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    style->fonts[i] = fonts_get_system_font(
        layout_font_key_for_role((WatchfaceFontRole)i, is_compact));
  }
}
