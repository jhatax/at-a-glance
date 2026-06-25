#include "date.h"
#include "substratum_renderer.h"

#define MAX_STR_LEN 16
static char s_date_buffer[MAX_STR_LEN] = {0};
static TextLayer *s_date_layer = NULL;
static WatchfaceColorRole s_date_color_role = WATCHFACE_COLOR_ROLE_DATE;

static void uppercase_date(char *buf);

static void uppercase_date(char *buf) {
  if (!buf || strlen(buf) == 0) {
    return;
  }

  int distance = 'a' - 'A';
  for (char *p = buf; *p; ++p) {
    if (*p >= 'a' && *p <= 'z') {
      *p = (char)(*p - distance);
    }
  }
}

bool date_module_create(Layer *root, const WatchfaceTextSubstratum *text, GFont font) {
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

void date_module_destroy(void) {
  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
  }

  s_date_buffer[0] = '\0';
  s_date_color_role = WATCHFACE_COLOR_ROLE_DATE;
}

void date_module_refresh(const ColorPalette *palette) {
  if (!s_date_layer || !palette) {
    return;
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  if (!t) {
    return;
  }

  strftime(s_date_buffer, MAX_STR_LEN, "%d%b", t);
  uppercase_date(s_date_buffer);
  substratum_renderer_update_text_layer(
      s_date_layer, s_date_buffer, substratum_renderer_color_for_role(palette, s_date_color_role));
}
