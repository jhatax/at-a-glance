#include "date.h"
#include "substratum_renderer.h"
#include "../c/ataglance.h"

static char s_date_buffer[ATAGLANCE_MAX_STR_LEN] = {0};
static TextLayer* s_date_layer = NULL;
static const WatchfaceSurface* s_surface = NULL;

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
    const WatchfaceSurface* surface) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  const WatchfaceTextSubstratum* text = &surface->date.text;
  s_date_layer = substratum_renderer_create_text_layer(
      root,
      text,
      surface->style.fonts[text->font_role]);
  if (!s_date_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create date text layer");
    return false;
  }

  s_surface = surface;
  return true;
}

void date_module_destroy(void) {
  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
  }

  s_date_buffer[0] = '\0';
  s_surface = NULL;
}

void date_module_refresh(const WatchfaceSurface* surface) {
  if (surface) {
    s_surface = surface;
  }
  if (!s_date_layer || !s_surface || !s_surface->style.palette) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  if (!t) {
    return;
  }

  strftime(s_date_buffer, ATAGLANCE_MAX_STR_LEN, "%a · %d %b", t);
  uppercase_date(s_date_buffer);
  substratum_renderer_update_text_layer(
      s_date_layer,
      s_date_buffer,
      substratum_renderer_color_for_role(
          s_surface->style.palette,
          s_surface->date.text.color_role));
}
