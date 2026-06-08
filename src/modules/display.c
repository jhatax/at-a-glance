#include "display.h"
#include "settings.h"

static const VisualPalette c_dark_palette = {
  .background = GColorBlack,
  .primary_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorRichBrilliantLavender, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .rule = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
};

static const VisualPalette c_light_palette = {
  .background = GColorWhite,
  .primary_text = GColorBlack,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorImperialPurple, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .rule = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorBlack),
};

/* Fallback palettes aren't needed. Its one struct or another, same size
static const VisualPalette c_fallback_dark_palette = {
  .background = GColorBlack,
  .primary_text = GColorWhite,
  .unavailable_text = GColorWhite,
  .date = GColorWhite,
  .time = GColorWhite,
  .rule = GColorWhite,
  .steps_icon = GColorWhite,
};

static const VisualPalette c_fallback_light_palette = {
  .background = GColorWhite,
  .primary_text = GColorBlack,
  .unavailable_text = GColorBlack,
  .date = GColorBlack,
  .time = GColorBlack,
  .rule = GColorBlack,
  .steps_icon = GColorBlack,
};
*/

const VisualPalette* display_get_palette(uint8_t display_mode) {
  if (display_mode == DISPLAY_MODE_LIGHT) {
    return &c_light_palette;
  }

  return &c_dark_palette;
}

void display_update_text_layer(
    TextLayer* layer,
    const char* text,
    GColor text_color) {
  if (!layer || !text) {
    return;
  }

  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_text(layer, text);
}
