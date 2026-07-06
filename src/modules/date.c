#include "date.h"

#include "substratum_renderer.h"

#define MAX_STR_LEN 16
static char s_date_buffer[MAX_STR_LEN] = {0};
static TextLayer* s_date_layer = NULL;
static WatchfaceColorRole s_date_color_role = WATCHFACE_COLOR_ROLE_DATE;

bool date_module_create(
  Layer* root,
  const WatchfaceTextSubstratum* text,
  GFont font) {
  if (!root || !text || !font) {
    return false;
  }

  s_date_layer = substratum_renderer_create_text_layer(root, text, font);
  if (!s_date_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create date text layer");
    return false;
  }

  // Don't need to save the font because it isn't changed post creation
  s_date_color_role = text->color_role;
  return true;
}

void date_module_destroy() {
  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
  }

  s_date_buffer[0] = '\0';
  s_date_color_role = WATCHFACE_COLOR_ROLE_DATE;
}

void date_module_refresh(
  const ColorPalette* palette) {
  // Every weekday is a constant char*
  // The array of these weekdays is also a static constant
  // Only this function needs to have visibility into this array
  static const char* const c_weekdays[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

  if (!s_date_layer || !palette) {
    return;
  }
  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  if (!t) {
    return;
  }

  snprintf(s_date_buffer, MAX_STR_LEN, "%s·", c_weekdays[t->tm_wday]);
  uint8_t len = (uint8_t)strlen(s_date_buffer);
  strftime(s_date_buffer + len, (ARRAY_LENGTH(s_date_buffer) - len), "%d·%b", t);

  // uppercase_date(s_date_buffer);
  substratum_renderer_update_text_layer(s_date_layer,
    s_date_buffer,
    substratum_renderer_color_for_role(palette, s_date_color_role));
}
