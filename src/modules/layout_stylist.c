#include "gcolor_definitions.h"
#include "layout.h"
#include "settings.h"
#include "layout_design.h"
// Stylist: Font lifecycle management

#define STYLIST_INVALID_FONT_RESOURCE_ID 0

static const char* layout_font_key_for_role(WatchfaceFontRole role, bool is_compact) {
  switch (role) {
    case WATCHFACE_FONT_ROLE_TIME:
      return is_compact ? DESIGN_FONT_TIME_COMPACT : DESIGN_FONT_TIME_FULL;
    case WATCHFACE_FONT_ROLE_TEXT:
    case WATCHFACE_FONT_ROLE_COUNT:
    default:
      return is_compact ? DESIGN_FONT_COMPACT_PRIMARY_TEXT : DESIGN_FONT_PRIMARY_TEXT;
  }
}

static uint32_t layout_custom_font_resource_id_for_role(WatchfaceFontRole role, bool is_compact) {
  switch (role) {
    case WATCHFACE_FONT_ROLE_TIME:
      return is_compact ?
          RESOURCE_ID_FONT_CABIN_TIME_36_SEMIBOLD : RESOURCE_ID_FONT_CABIN_TIME_60_SEMIBOLD;
    case WATCHFACE_FONT_ROLE_TEXT:
      return is_compact ?
          RESOURCE_ID_FONT_CABIN_TEXT_14_MEDIUM : RESOURCE_ID_FONT_CABIN_TEXT_18_MEDIUM;
    case WATCHFACE_FONT_ROLE_COUNT:
    default:
      return STYLIST_INVALID_FONT_RESOURCE_ID;
  }
}

static GFont watchface_find_loaded_custom_font(
    WatchfaceSurfaceStyle* style,
    uint8_t max,
    uint32_t id_to_match,
    GFont* custom_fonts) {
  // Guard against NULL & out-of-bounds access
  if (!style || max > WATCHFACE_FONT_ROLE_COUNT || !custom_fonts) {
    return NULL;
  }

  for (uint8_t i = 0; i < max; ++i) {
    GFont current = custom_fonts[i];
    uint32_t current_resource = style->custom_font_resource_ids[i];
    if (current && id_to_match == current_resource) {
      return current;
    }
  }

  return NULL;
}

bool layout_watchface_initialize_fonts(WatchfaceSurfaceStyle* style) {
  if (!style) {
    return false;
  }

  bool is_compact = style->is_compact;

  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    style->system_fonts[i] = fonts_get_system_font(
        layout_font_key_for_role((WatchfaceFontRole)i, is_compact));

    style->custom_font_resource_ids[i] =
        layout_custom_font_resource_id_for_role((WatchfaceFontRole)i, is_compact);
  }

  return true;
}

bool layout_watchface_load_custom_fonts(
    WatchfaceSurfaceStyle* style,
    GFont* custom_fonts) {

  if (!style || !custom_fonts) {
    return false;
  }

  GFont unique_customs[WATCHFACE_FONT_ROLE_COUNT] = {NULL};
  bool outcome = false;
  uint8_t j = 0;
  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {

    // 1. Get the custom font resource-id
    const uint32_t c_resource = style->custom_font_resource_ids[i];
    if (c_resource == STYLIST_INVALID_FONT_RESOURCE_ID) {
      continue;
    }

    GFont font = watchface_find_loaded_custom_font(style, i, c_resource, custom_fonts);

    // This font might already be assigned to another role.
    // Added it to our shared array of fonts
    if (font != NULL) {
      custom_fonts[i] = font;
      outcome = true;
      continue;
    }

    custom_fonts[i] = fonts_load_custom_font(resource_get_handle(c_resource));
    if (custom_fonts[i] == NULL) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to load watchface custom font resource %u", c_resource);
    } else {
      unique_customs[j++] = custom_fonts[i];
      outcome = true;
    }
    // We continue as there are fallback system fonts that are always selected
  }

  if (outcome) {
    for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
      if (custom_fonts[i]) {
        style->system_fonts[i] = custom_fonts[i];
      }
      // custom_fonts stops being role-indexed here. From this point on it is
      // a compact ownership list of unique custom fonts for unload.
      custom_fonts[i] = unique_customs[i];
    }
  }
  return outcome;
}

void layout_watchface_unload_custom_fonts(GFont* custom_fonts) {
  if (!custom_fonts) {
    return;
  }
  // Unload all custom fonts in the input array
  GFont current = NULL;
  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    current = custom_fonts[i];
    if (current) {
      fonts_unload_custom_font(current);
      custom_fonts[i] = NULL;
    }
  }
}

// Stylist: Color Palette
static const ColorPalette c_dark_palette = {
  .is_light_mode = false,
  .background = PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
  .primary_text = GColorWhite,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .is_light_mode = true,
  .background = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorOrange, GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack),
  .primary_text = GColorBlack,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack),
};

// API Contract: If style is a valid value, the palette will always be non-NULL
// Invariant satisfied because the value assigned to palette is a pointer to
// one of two static-constant structs with module-scope.
void layout_watchface_update_palette(WatchfaceSurfaceStyle* style, uint8_t display_mode) {
  if (!style) {
    return;
  }

  style->palette = (display_mode == DISPLAY_MODE_DARK) ? &c_dark_palette : &c_light_palette;
}
