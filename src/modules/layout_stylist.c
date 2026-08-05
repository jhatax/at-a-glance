#include "helper.h"
#include "layout_style.h"
#include "settings.h"
#include "watchface_layout.h"

// Stylist: Font lifecycle management
#if IS_LARGE_DISPLAY
// Full displays
// System fonts
#define DESIGN_TIME_FONT_KEY FONT_KEY_ROBOTO_BOLD_SUBSET_49
#define DESIGN_TEXT_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define DESIGN_DATE_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define DESIGN_LOCATION_FONT_KEY FONT_KEY_GOTHIC_18
// Custom fonts
#ifdef PBL_PLATFORM_EMERY
#define CUSTOM_FONT_TIME_RESOURCE RESOURCE_ID_FONT_CABIN_MEDIUM_58
#else
#define CUSTOM_FONT_TIME_RESOURCE RESOURCE_ID_FONT_CABIN_MEDIUM_70
#endif
#define CUSTOM_FONT_DATE_RESOURCE RESOURCE_ID_FONT_DATE_TEXT_22
#define CUSTOM_FONT_TEXT_RESOURCE CUSTOM_FONT_DATE_RESOURCE
#define CUSTOM_FONT_LOCATION_RESOURCE RESOURCE_ID_FONT_LOCATION_16
#else
// Compact displays
// System fonts
#define DESIGN_TIME_FONT_KEY FONT_KEY_BITHAM_42_MEDIUM_NUMBERS
#define DESIGN_TEXT_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
#define DESIGN_DATE_FONT_KEY DESIGN_TEXT_FONT_KEY
#define DESIGN_DATE_LOCATION_KEY FONT_KEY_GOTHIC_14
// Custom fonts
#define CUSTOM_FONT_TIME_RESOURCE RESOURCE_ID_FONT_CABIN_MEDIUM_42
#define CUSTOM_FONT_DATE_RESOURCE RESOURCE_ID_FONT_DATE_TEXT_16
#define CUSTOM_FONT_TEXT_RESOURCE CUSTOM_FONT_DATE_RESOURCE
#define CUSTOM_FONT_LOCATION_RESOURCE RESOURCE_ID_FONT_LOCATION_14
#endif

#define STYLIST_INVALID_FONT_RESOURCE_ID 0

static const char* layout_font_key_for_role(
    WatchfaceFontRole role) {
  switch (role) {
    case WATCHFACE_FONT_ROLE_TIME:
      return DESIGN_TIME_FONT_KEY;
    case WATCHFACE_FONT_ROLE_DATE:
      return DESIGN_DATE_FONT_KEY;
    case WATCHFACE_FONT_ROLE_TEXT:
    case WATCHFACE_FONT_ROLE_COUNT:
    default:
      return DESIGN_TEXT_FONT_KEY;
  }
}

static uint32_t layout_custom_font_resource_id_for_role(
    WatchfaceFontRole role) {
  switch (role) {
    case WATCHFACE_FONT_ROLE_TIME:
      return CUSTOM_FONT_TIME_RESOURCE;
    case WATCHFACE_FONT_ROLE_DATE:
      return CUSTOM_FONT_DATE_RESOURCE;
    case WATCHFACE_FONT_ROLE_TEXT:
      return CUSTOM_FONT_TEXT_RESOURCE;
    case WATCHFACE_FONT_ROLE_LOCATION:
      return CUSTOM_FONT_LOCATION_RESOURCE;
    case WATCHFACE_FONT_ROLE_COUNT:
    default:
      return STYLIST_INVALID_FONT_RESOURCE_ID;
  }
}

static GFont layout_find_loaded_custom_font(
    FontBook* fontbook,
    uint8_t max,
    uint32_t id_to_match) {
  // Guard against NULL & out-of-bounds access
  if (max > WATCHFACE_FONT_ROLE_COUNT || fontbook == NULL) {
    return NULL;
  }

  GFont c_font = NULL;
  GFont* custom_fonts = fontbook->custom_fonts;
  const uint32_t* custom_ids = fontbook->custom_font_resource_ids;
  if (custom_ids != NULL && custom_fonts != NULL) {
    uint32_t c_resource = STYLIST_INVALID_FONT_RESOURCE_ID;
    uint8_t i = 0;

    // DO NOT CHANGE THIS DO..WHILE loop.
    // This helps you check the first entry in the custom_fonts row
    // when you pass in max = 0.
    // Alternatively, increase the value of max by 1 before the for..loop
    do {
      // Find the font in the shared font array
      c_font = custom_fonts[i];
      c_resource = custom_ids[i];

      // This check for whether c_font is NULL protects you from NULL deref
      // if the custom_fonts array is empty
      if (c_font != NULL && id_to_match == c_resource) {
        break;
      }
      // The id didn't match, so reset the temporary font
      // otherwise, the last retrieved custom font will be returned
      // even if it didn't match the resource_id
      c_font = NULL;
      ++i;
    } while (i < max);
  }

  return c_font;
}

bool layout_watchface_initialize_fonts(
    FontBook* fontbook) {
  if (!fontbook) {
    return false;
  }

  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    // Start by selecting system fonts; custom fonts will replace these fonts
    fontbook->chosen_fonts[i] =
        fonts_get_system_font(layout_font_key_for_role((WatchfaceFontRole)i));

    fontbook->custom_font_resource_ids[i] =
        layout_custom_font_resource_id_for_role((WatchfaceFontRole)i);
  }

  return true;
}

bool layout_watchface_load_custom_fonts(
    FontBook* fontbook) {
  if (fontbook == NULL) {
    return false;
  }

  // We need to trim fontbook->custom_fonts at the end to prevent null-ptr dereference or
  // double-free in watchface_destroy(). unique_customs helps with that after system_fonts
  // and custom_fonts have been consolidated into one list
  GFont unique_customs[WATCHFACE_FONT_ROLE_COUNT] = {0};
  bool outcome = false;
  uint8_t j = 0;
  uint32_t* custom_font_resource_ids = fontbook->custom_font_resource_ids;
  GFont* custom_fonts = fontbook->custom_fonts;
  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    // 1. Get the custom font resource-id
    const uint32_t c_resource = custom_font_resource_ids[i];
    if (c_resource == STYLIST_INVALID_FONT_RESOURCE_ID) {
      continue;
    }

    // Find out if this font has been loaded in the past
    // If the font is on this list, it is already in unique_customs
    GFont font = layout_find_loaded_custom_font(fontbook, i, c_resource);

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
      // This is the first time this font was loaded. it should be added to
      // unique_customs so that it can be safely released during watchface_destroy
      unique_customs[j++] = custom_fonts[i];
      outcome = true;
    }
    // We continue as there are fallback system fonts that are always selected
  }

  // the number of fonts we loaded is saved in "j"
  // Save this value in the variable to satisfy the invariant
  fontbook->custom_fonts_loaded_count = j;

  if (outcome) {
    for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
      if (custom_fonts[i]) {
        fontbook->chosen_fonts[i] = custom_fonts[i];
      }

      // custom_fonts stops being role-indexed here. From this point on it is
      // a compact ownership list of unique custom fonts for unload.
      custom_fonts[i] = unique_customs[i];
    }
  }
  return outcome;
}

void layout_watchface_unload_custom_fonts(
    FontBook* fontbook) {
  if (!fontbook) {
    return;
  }

  // Unload all custom fonts in the input array
  GFont current = NULL;
  for (uint8_t i = 0; i < fontbook->custom_fonts_loaded_count; ++i) {
    current = fontbook->custom_fonts[i];
    if (current) {
      fonts_unload_custom_font(current);
      fontbook->custom_fonts[i] = NULL;
    }
  }
  // Reset this to be zero
  fontbook->custom_fonts_loaded_count = 0;
}

// Stylist: Color Palettes
#ifdef PBL_COLOR
static const ColorPalette color_dark_palette = {
    .is_light_mode = false,
    .background = GColorOxfordBlue,
    .time_text = GColorCeleste,
    .date_text = GColorCeleste,
    .primary_text = GColorWhite,
    .outofrange_text = GColorLightGray,
};

static const ColorPalette color_light_palette = {
    .is_light_mode = true,
    .background = GColorCeleste,
    .time_text = GColorOxfordBlue,
    .date_text = GColorOxfordBlue,
    .primary_text = GColorBlack,
    .outofrange_text = GColorDarkGray,
};
#endif

static const ColorPalette mono_dark_palette = {
    .is_light_mode = false,
    .background = GColorBlack,
    .time_text = GColorWhite,
    .date_text = GColorWhite,
    .primary_text = GColorWhite,
    .outofrange_text = GColorWhite,
};

static const ColorPalette mono_light_palette = {
    .is_light_mode = true,
    .background = GColorWhite,
    .time_text = GColorBlack,
    .date_text = GColorBlack,
    .primary_text = GColorBlack,
    .outofrange_text = GColorBlack,
};

// API Contract: If style is a valid value, the palette will always be non-NULL
// Invariant satisfied because the value assigned to palette is a pointer to
// one of two static-constant structs with module-scope.
void layout_watchface_update_palette(
    WatchfaceSurfaceStyle* style,
    SupportedDisplayModes display_mode) {
  if (!style) {
    return;
  }
  const ColorPalette* pal = NULL;
  switch (display_mode) {
#ifdef PBL_COLOR
    case DISPLAY_MODE_LIGHT_COLOR:
      pal = &color_light_palette;
      break;
    case DISPLAY_MODE_DARK_COLOR:
      pal = &color_dark_palette;
      break;
#endif
    case DISPLAY_MODE_DARK_MONOCHROME:
      pal = &mono_dark_palette;
      break;
    case DISPLAY_MODE_LIGHT_MONOCHROME:
    default:
      pal = &mono_light_palette;
      break;
  }
  style->palette = pal;
}
