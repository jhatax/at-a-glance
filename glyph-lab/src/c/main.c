#include <pebble.h>

#define GLYPH_GRID_SIZE 28
#define GLYPH_COL_COUNT 2

typedef enum {
  GLYPH_THEME_DARK = 0,
  GLYPH_THEME_LIGHT,
} GlyphTheme;

typedef enum {
  GLYPH_RENDER_BW = 0,
  GLYPH_RENDER_COLOR,
} GlyphRenderMode;

typedef struct {
  GColor background;
  GColor primary;
  GColor muted;
  GColor steps;
} GlyphPalette;

typedef void (*GlyphDrawProc)(
    GContext* ctx,
    GRect frame,
    const GlyphPalette* palette);

static Window* s_window;
static Layer* s_canvas_layer;
static GFont s_status_font;
static GlyphTheme s_theme = GLYPH_THEME_DARK;
static GlyphRenderMode s_render_mode = GLYPH_RENDER_BW;
static GPath* s_shoe_body_path;

static const GPathInfo SHOE_BODY_PATH_INFO = {
  .num_points = 5,
  .points = (GPoint[]) {
    {4, 16},
    {10, 4},
    {24, 16},
    {25, 21},
    {8, 21},
  },
};

static int16_t glyph_min(int16_t a, int16_t b) {
  return a < b ? a : b;
}

static int16_t glyph_draw_size(GRect frame) {
  return glyph_min(frame.size.w, frame.size.h);
}

static int16_t glyph_scale(GRect frame, int16_t value) {
  return (value * glyph_draw_size(frame)) / GLYPH_GRID_SIZE;
}

static int16_t glyph_x(GRect frame, int16_t value) {
  return frame.origin.x + ((value * frame.size.w) / GLYPH_GRID_SIZE);
}

static int16_t glyph_y(GRect frame, int16_t value) {
  return frame.origin.y + ((value * frame.size.h) / GLYPH_GRID_SIZE);
}

static GPoint glyph_point(GRect frame, int16_t x, int16_t y) {
  return GPoint(glyph_x(frame, x), glyph_y(frame, y));
}

static GRect glyph_rect(
    GRect frame,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h) {
  return GRect(glyph_x(frame, x),
               glyph_y(frame, y),
               (w * frame.size.w) / GLYPH_GRID_SIZE,
               (h * frame.size.h) / GLYPH_GRID_SIZE);
}

static void glyph_line(
    GContext* ctx,
    GRect frame,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1) {
  graphics_draw_line(ctx,
                     glyph_point(frame, x0, y0),
                     glyph_point(frame, x1, y1));
}

static void fill_circle(
    GContext* ctx,
    GRect frame,
    int16_t x,
    int16_t y,
    int16_t r) {
  graphics_fill_circle(ctx,
                       glyph_point(frame, x, y),
                       glyph_scale(frame, r));
}

static GColor glyph_ink_color(const GlyphPalette* palette) {
  if (s_render_mode == GLYPH_RENDER_COLOR) {
    return palette->steps;
  }

  return palette->primary;
}

static void select_palette(GlyphPalette* palette) {
  bool dark = s_theme == GLYPH_THEME_DARK;
  bool color = s_render_mode == GLYPH_RENDER_COLOR;

  palette->background = dark ? GColorBlack : GColorWhite;
  palette->primary = dark ? GColorWhite : GColorBlack;
  palette->muted = dark ? GColorLightGray : GColorDarkGray;
  palette->steps = color ?
      PBL_IF_COLOR_ELSE(GColorChromeYellow, palette->primary) :
      palette->primary;
}

static void draw_tread_icon(
    GContext* ctx,
    GRect frame,
    const GlyphPalette* palette) {
  GColor color = glyph_ink_color(palette);

  graphics_context_set_fill_color(ctx, color);

  fill_circle(ctx, frame, 14, 9, 7);

  graphics_context_set_fill_color(ctx, palette->background);
  graphics_fill_rect(ctx, glyph_rect(frame, 6, 15, 16, 4),
                     0, GCornerNone);

  graphics_context_set_fill_color(ctx, color);
  fill_circle(ctx, frame, 14, 22, 4);
}

static void draw_shoe_icon(
    GContext* ctx,
    GRect frame,
    const GlyphPalette* palette) {
  GColor color = glyph_ink_color(palette);

  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 3);

  gpath_move_to(s_shoe_body_path, frame.origin);
  gpath_draw_filled(ctx, s_shoe_body_path);

  fill_circle(ctx, frame, 6, 15, 4);
  glyph_line(ctx, frame, 3, 23, 25, 23);
}

static GlyphDrawProc proc_for_col(int col) {
  static GlyphDrawProc procs[] = {
    draw_tread_icon,
    draw_shoe_icon,
  };

  return procs[col];
}

static void draw_status(
    GContext* ctx,
    GRect bounds,
    const GlyphPalette* palette) {
  char text[32];

  snprintf(text,
           sizeof(text),
           "%s %s STEP",
           s_render_mode == GLYPH_RENDER_COLOR ? "COLOR" : "BW",
           s_theme == GLYPH_THEME_DARK ? "DARK" : "LIGHT");

  graphics_context_set_text_color(ctx, palette->muted);
  graphics_draw_text(ctx,
                     text,
                     s_status_font,
                     GRect(4, 0, bounds.size.w - 8, 18),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft,
                     NULL);
}

static void canvas_update_proc(Layer* layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  GlyphPalette palette;
  int top = 18;
  int cell_w = bounds.size.w / GLYPH_COL_COUNT;
  int cell_h = bounds.size.h - top;
  int icon_size = glyph_min(cell_w - 8, cell_h - 8);

  if (icon_size > GLYPH_GRID_SIZE) {
    icon_size = GLYPH_GRID_SIZE;
  }

  select_palette(&palette);
  graphics_context_set_fill_color(ctx, palette.background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_status(ctx, bounds, &palette);

  for (int col = 0; col < GLYPH_COL_COUNT; ++col) {
    int x = (cell_w * col) + ((cell_w - icon_size) / 2);
    int y = top + ((cell_h - icon_size) / 2);
    GRect frame = GRect(x, y, icon_size, icon_size);
    GlyphDrawProc proc = proc_for_col(col);
    proc(ctx, frame, &palette);
  }
}

static void mark_canvas_dirty(void) {
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void select_click_handler(
    ClickRecognizerRef recognizer,
    void* ctx) {
  (void)recognizer;
  (void)ctx;

  s_render_mode = s_render_mode == GLYPH_RENDER_BW ?
      GLYPH_RENDER_COLOR : GLYPH_RENDER_BW;
  mark_canvas_dirty();
}

static void up_click_handler(
    ClickRecognizerRef recognizer,
    void* ctx) {
  (void)recognizer;
  (void)ctx;

  s_theme = s_theme == GLYPH_THEME_DARK ?
      GLYPH_THEME_LIGHT : GLYPH_THEME_DARK;
  mark_canvas_dirty();
}

static void click_config_provider(void* ctx) {
  (void)ctx;

  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}

static void main_window_load(Window* window) {
  Layer* root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_shoe_body_path = gpath_create(&SHOE_BODY_PATH_INFO);
  if (!s_shoe_body_path) {
    return;
  }

  s_canvas_layer = layer_create(bounds);
  if (!s_canvas_layer) {
    gpath_destroy(s_shoe_body_path);
    s_shoe_body_path = NULL;
    return;
  }

  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);
}

static void main_window_unload(Window* window) {
  (void)window;

  if (s_canvas_layer) {
    layer_destroy(s_canvas_layer);
    s_canvas_layer = NULL;
  }

  if (s_shoe_body_path) {
    gpath_destroy(s_shoe_body_path);
    s_shoe_body_path = NULL;
  }
}

static void init(void) {
  s_status_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_window = window_create();
  if (!s_window) {
    return;
  }

  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_window, true);
}

static void deinit(void) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
