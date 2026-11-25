#include "time.h"

#include "settings.h"
#include "substratum_renderer.h"

#define MAX_STR_LEN 8
static char s_time_buffer[MAX_STR_LEN] = {0};
static TextLayer* s_time_layer = NULL;
static WatchfaceColorRole s_time_color_role = WATCHFACE_COLOR_ROLE_TIME;

static void format_time(
    char* buf,
    size_t buflen,
    const struct tm* t,
    bool is_24_h);

static void format_time(
    char* buf,
    size_t buflen,
    const struct tm* t,
    bool is_24_h) {
  if (!buf || buflen == 0 || !t) {
    return;
  }

  if (is_24_h) {
    strftime(buf, buflen, "%H:%M", t);
  } else {
    strftime(buf, buflen, "%I:%M", t);
    if (buf[0] == '0') {
      memmove(buf, buf + 1, strlen(buf));
    }
  }
}

bool time_module_create(
    Layer* root,
    const WatchfaceTextSubstratum* text,
    GFont font) {
  if (!root || !text || !font) {
    return false;
  }

  s_time_layer = substratum_renderer_create_text_layer(root, text, font);
  if (!s_time_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create time text layer");
    return false;
  }

  s_time_color_role = text->color_role;
  return true;
}

void time_module_destroy() {
  if (s_time_layer) {
    text_layer_destroy(s_time_layer);
    s_time_layer = NULL;
  }

  s_time_buffer[0] = '\0';
  s_time_color_role = WATCHFACE_COLOR_ROLE_TIME;
}

void time_module_refresh(
    const ColorPalette* palette,
    bool is_24_h) {
  if (!s_time_layer || !palette) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  if (!t) {
    return;
  }

  format_time(s_time_buffer, MAX_STR_LEN, t, is_24_h);
  substratum_renderer_update_text_layer(
      s_time_layer,
      s_time_buffer,
      substratum_renderer_color_for_role(palette, s_time_color_role));
}
