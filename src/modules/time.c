#include "time.h"
#include "settings.h"
#include "../c/ataglance.h"

static char s_time_buffer[ATAGLANCE_MAX_STR_LEN];
static TextLayer* s_time_layer;
static const VisualPalette* s_palette;

static void format_time(
    char* buf,
    size_t buflen,
    const struct tm* t,
    uint8_t time_format);

static void format_time(
    char* buf,
    size_t buflen,
    const struct tm* t,
    uint8_t time_format) {
  if (!buf || buflen == 0 || !t) {
    return;
  }

  if (time_format == TIME_FMT_12) {
    strftime(buf, buflen, "%I:%M", t);
    if (buf[0] == '0') {
      memmove(buf, buf + 1, strlen(buf));
    }
    return;
  }

  strftime(buf, buflen, "%H:%M", t);
}

bool time_module_create(
    Layer* root,
    const GRect* frame,
    const VisualPalette* palette) {
  if (!root || !frame || !palette) {
    return false;
  }

  s_palette = palette;
  s_time_layer = text_layer_create(*frame);
  if (!s_time_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create time text layer");
    return false;
  }

  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(
      s_time_layer,
      fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_time_layer));
  return true;
}

void time_module_destroy(void) {
  if (s_time_layer) {
    text_layer_destroy(s_time_layer);
    s_time_layer = NULL;
  }

  s_time_buffer[0] = '\0';
}

void time_module_refresh(
    uint8_t time_format,
    const VisualPalette* palette) {
  if (palette) {
    s_palette = palette;
  }
  if (!s_time_layer || !s_palette) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  if (!t) {
    return;
  }

  format_time(s_time_buffer, ATAGLANCE_MAX_STR_LEN, t, time_format);
  display_update_text_layer(s_time_layer, s_time_buffer, s_palette->time);
}
