#include "time.h"
#include "settings.h"
#include "../c/ataglance.h"

static char s_time_buffer[ATAGLANCE_MAX_STR_LEN];
static TextLayer* s_time_layer;
static GFont s_custom_time_font;
static const WatchfaceSurface* s_surface;

static void format_time(
    char* buf,
    size_t buflen,
    const struct tm* t,
    uint8_t time_format);
static uint32_t custom_time_font_resource_id(
    const WatchfaceSurface* surface);

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

static uint32_t custom_time_font_resource_id(
    const WatchfaceSurface* surface) {
  if (!surface) {
    return 0;
  }

  if (surface->face_width >= ATAGLANCE_DESIGN_FACE_WIDTH &&
      surface->face_height >= ATAGLANCE_DESIGN_FACE_HEIGHT) {
    return RESOURCE_ID_FONT_TIME_UNBOUNDED_48;
  }

  return 0;
}

bool time_module_create(
    Layer* root,
    const WatchfaceSurface* surface) {
  if (!root || !surface || !surface->style.palette) {
    return false;
  }

  const WatchfaceTextSubstratum* text = &surface->time.text;
  s_surface = surface;
  s_time_layer = text_layer_create(text->frame);
  if (!s_time_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Failed to create time text layer");
    return false;
  }

  text_layer_set_background_color(s_time_layer, GColorClear);
  const uint32_t custom_font_resource_id =
      custom_time_font_resource_id(surface);
  if (custom_font_resource_id) {
    s_custom_time_font = fonts_load_custom_font(
        resource_get_handle(custom_font_resource_id));
  }

  text_layer_set_font(
      s_time_layer,
      s_custom_time_font ?
          s_custom_time_font :
          surface->style.fonts[text->font_role]);
  text_layer_set_text_alignment(s_time_layer, text->alignment);
  layer_add_child(root, text_layer_get_layer(s_time_layer));
  return true;
}

void time_module_destroy(void) {
  if (s_time_layer) {
    text_layer_destroy(s_time_layer);
    s_time_layer = NULL;
  }

  if (s_custom_time_font) {
    fonts_unload_custom_font(s_custom_time_font);
    s_custom_time_font = NULL;
  }

  s_time_buffer[0] = '\0';
  s_surface = NULL;
}

void time_module_refresh(
    const WatchfaceSurface* surface,
    uint8_t time_format) {
  if (surface) {
    s_surface = surface;
  }
  if (!s_time_layer || !s_surface || !s_surface->style.palette) {
    return;
  }

  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  if (!t) {
    return;
  }

  format_time(s_time_buffer, ATAGLANCE_MAX_STR_LEN, t, time_format);
  layout_update_text_layer(
      s_time_layer,
      s_time_buffer,
      layout_color_for_role(
          s_surface->style.palette,
          s_surface->time.text.color_role));
}
