#include "date.h"
#include "../c/ataglance.h"

static char s_date_buffer[ATAGLANCE_MAX_STR_LEN];
static TextLayer* s_date_layer;
static const VisualPalette* s_palette;

static void uppercase_date(char* buf);

static void uppercase_date(char* buf) {
  if (!buf || strlen(buf) == 0) {
    return;
  }

  int distance = 'a' - 'A';
  for (char* p = buf; *p; ++p) {
    if (*p >= 'a' && *p <= 'z') {
      *p = (char)(*p - distance);
    }
  }
}

bool date_module_create(
    Layer* root,
    const GRect* frame,
    const VisualPalette* palette) {
  if (!root || !frame || !palette) {
    return false;
  }

  s_palette = palette;
  s_date_layer = text_layer_create(*frame);
  if (!s_date_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create date text layer");
    return false;
  }

  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(
      s_date_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_date_layer));
  return true;
}

void date_module_destroy(void) {
  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
  }

  s_date_buffer[0] = '\0';
}

void date_module_refresh(const VisualPalette* palette) {
  if (palette) {
    s_palette = palette;
  }
  if (!s_date_layer || !s_palette) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  if (!t) {
    return;
  }

  strftime(s_date_buffer, ATAGLANCE_MAX_STR_LEN, "%a · %d %b", t);
  uppercase_date(s_date_buffer);
  display_update_text_layer(
      s_date_layer,
      s_date_buffer,
      s_palette->date);
}
