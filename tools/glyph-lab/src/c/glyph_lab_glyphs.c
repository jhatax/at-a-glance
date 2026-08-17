#include "glyph_lab_glyphs.h"

#include "../../../src/modules/climate_glyphs.h"
#include "../../../src/modules/helper_computations.h"

bool glyph_lab_glyphs_init() { return true; }

void glyph_lab_glyphs_deinit() {}

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

void glyph_lab_select_palette(
    ColorPalette* palette,
    uint8_t palette_index) {
  if (!palette) {
    return;
  }

  if (palette_index == 1 || palette_index == 3) {
    *palette = (ColorPalette){
        .is_light_mode = true,
        .background =
            PBL_IF_COLOR_ELSE(palette_index == 3 ? GColorWhite : GColorCeleste, GColorWhite),
        .background_layer_background =
            PBL_IF_COLOR_ELSE(palette_index == 3 ? GColorWhite : GColorCeleste, GColorWhite),
        .background_layer_rule = PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack),
        .primary_text = GColorBlack,
        .outofrange_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack),
        .date = PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack),
        .time = PBL_IF_COLOR_ELSE(GColorOrange, GColorBlack),
    };
    return;
  }

  *palette = (ColorPalette){
      .is_light_mode = false,
      .background =
          PBL_IF_COLOR_ELSE(palette_index == 2 ? GColorBlack : GColorOxfordBlue, GColorBlack),
      .background_layer_background =
          PBL_IF_COLOR_ELSE(palette_index == 2 ? GColorBlack : GColorOxfordBlue, GColorBlack),
      .background_layer_rule = GColorWhite,
      .primary_text = GColorWhite,
      .outofrange_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
      .date = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
      .time = PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite),
  };
}

void glyph_lab_draw_climate_icon(
    GContext* ctx,
    const GRect* frame,
    int16_t weather_condition,
    bool is_day,
    const ColorPalette* palette) {
  if (!ctx || !frame || !palette) {
    return;
  }

  ClimatePalette climate_palette =
      palette->is_light_mode ? c_light_climate_palette : c_dark_climate_palette;
  climate_palette = (ClimatePalette){
      .background = palette->background,
      .normal = palette->primary_text,
      .outofrange = palette->outofrange_text,
      .sun = climate_palette.sun,
      .cold = climate_palette.cold,
      .cloud = climate_palette.cloud,
      .clear_ring = climate_palette.clear_ring,
      .clear_fill = climate_palette.clear_fill,
  };

  draw_climate_icon(ctx, frame, weather_condition, is_day, &climate_palette);
}
