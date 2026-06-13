#include <pebble.h>

#include "glyph_lab_glyphs.h"
#include "glyph_lab_helper.h"

#define GLYPH_COL_COUNT 2
#define GLYPH_ROW_COUNT 2
#define STATUS_HEIGHT 18
#define LABEL_HEIGHT 14
#define OUTER_PADDING_RECT 6
#define OUTER_PADDING_ROUND 12
#define CELL_PADDING 4

typedef enum {
  GLYPH_KIND_BATTERY = 0,
  GLYPH_KIND_WEATHER,
  GLYPH_KIND_STEPS,
  GLYPH_KIND_BPM,
} GlyphKind;

typedef struct {
  const char* label;
  GlyphKind kind;
  int value;
  bool flag;
} GlyphCell;

typedef struct {
  const char* title;
  GlyphCell cells[GLYPH_COL_COUNT * GLYPH_ROW_COUNT];
} GlyphPage;

static Window* s_window;
static Layer* s_canvas_layer;
static GFont s_status_font;
static GFont s_label_font;
static bool s_is_light_mode = false;
static int s_page_index = 0;

static const GlyphPage GLYPH_PAGES[] = {
  {
    .title = "OVERVIEW",
    .cells = {
      {"BAT 74", GLYPH_KIND_BATTERY, 74, false},
      {"W 02", GLYPH_KIND_WEATHER, 2, false},
      {"STEPS", GLYPH_KIND_STEPS, 1, true},
      {"BPM 72", GLYPH_KIND_BPM, 72, true},
    },
  },
  {
    .title = "BATTERY",
    .cells = {
      {"100", GLYPH_KIND_BATTERY, 100, false},
      {"45", GLYPH_KIND_BATTERY, 45, false},
      {"18", GLYPH_KIND_BATTERY, 18, false},
      {"CHG", GLYPH_KIND_BATTERY, 43, true},
    },
  },
  {
    .title = "WEATHER 1",
    .cells = {
      {"CLEAR", GLYPH_KIND_WEATHER, 0, false},
      {"SUN", GLYPH_KIND_WEATHER, 1, false},
      {"PART", GLYPH_KIND_WEATHER, 2, false},
      {"CLOUD", GLYPH_KIND_WEATHER, 3, false},
    },
  },
  {
    .title = "WEATHER 2",
    .cells = {
      {"FOG", GLYPH_KIND_WEATHER, 45, false},
      {"DRIZ", GLYPH_KIND_WEATHER, 51, false},
      {"RAIN", GLYPH_KIND_WEATHER, 61, false},
      {"H RAIN", GLYPH_KIND_WEATHER, 65, false},
    },
  },
  {
    .title = "WEATHER 3",
    .cells = {
      {"SLEET", GLYPH_KIND_WEATHER, 56, false},
      {"SNOW", GLYPH_KIND_WEATHER, 71, false},
      {"SHOW", GLYPH_KIND_WEATHER, 80, false},
      {"H SHOW", GLYPH_KIND_WEATHER, 82, false},
    },
  },
  {
    .title = "WEATHER 4",
    .cells = {
      {"S SHOW", GLYPH_KIND_WEATHER, 85, false},
      {"STORM", GLYPH_KIND_WEATHER, 95, false},
      {"UNAV", GLYPH_KIND_WEATHER, -1, false},
      {"CLEAR", GLYPH_KIND_WEATHER, 0, false},
    },
  },
  {
    .title = "HEALTH",
    .cells = {
      {"STEPS", GLYPH_KIND_STEPS, 1, true},
      {"NO STEP", GLYPH_KIND_STEPS, 0, false},
      {"BPM 72", GLYPH_KIND_BPM, 72, true},
      {"BPM HI", GLYPH_KIND_BPM, 128, true},
    },
  },
  {
    .title = "UNAVAIL",
    .cells = {
      {"LOW", GLYPH_KIND_BATTERY, 9, false},
      {"UNAV", GLYPH_KIND_WEATHER, -1, false},
      {"NO STEP", GLYPH_KIND_STEPS, 0, false},
      {"NO BPM", GLYPH_KIND_BPM, 0, false},
    },
  },
};

static const int GLYPH_PAGE_COUNT =
    sizeof(GLYPH_PAGES) / sizeof(GLYPH_PAGES[0]);

static void mark_canvas_dirty(void) {
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void draw_status(
    GContext* ctx,
    const GRect* bounds,
    const ColorPalette* palette) {
  char status[32];

  snprintf(
      status,
      sizeof(status),
      "%d/%d %s %s",
      s_page_index + 1,
      GLYPH_PAGE_COUNT,
      GLYPH_PAGES[s_page_index].title,
      s_is_light_mode ? "LIGHT" : "DARK");

  graphics_context_set_text_color(ctx, palette->primary_text);
  graphics_draw_text(
      ctx,
      status,
      s_status_font,
      GRect(bounds->origin.x,
            bounds->origin.y,
            bounds->size.w,
            STATUS_HEIGHT),
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter,
      NULL);
}

static void draw_cell_label(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    const char* label) {
  graphics_context_set_text_color(ctx, palette->unavailable_text);
  graphics_draw_text(
      ctx,
      label,
      s_label_font,
      *frame,
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter,
      NULL);
}

static void draw_glyph_cell(
    GContext* ctx,
    const GRect* cell_frame,
    const GlyphCell* cell,
    const ColorPalette* palette) {
  GRect icon_area = GRect(
      cell_frame->origin.x + CELL_PADDING,
      cell_frame->origin.y + CELL_PADDING,
      cell_frame->size.w - (CELL_PADDING * 2),
      cell_frame->size.h - LABEL_HEIGHT - (CELL_PADDING * 2));
  int16_t icon_size = HELPER_MIN(icon_area.size.w, icon_area.size.h);
  GRect icon_frame = GRect(
      icon_area.origin.x + ((icon_area.size.w - icon_size) / 2),
      icon_area.origin.y + ((icon_area.size.h - icon_size) / 2),
      icon_size,
      icon_size);
  GRect label_frame = GRect(
      cell_frame->origin.x + 2,
      cell_frame->origin.y + cell_frame->size.h - LABEL_HEIGHT,
      cell_frame->size.w - 4,
      LABEL_HEIGHT);

  switch (cell->kind) {
    case GLYPH_KIND_BATTERY:
      glyph_lab_draw_battery_icon(
          ctx,
          &icon_frame,
          palette,
          s_is_light_mode,
          cell->value,
          cell->flag);
      break;
    case GLYPH_KIND_WEATHER:
      glyph_lab_draw_climate_icon(
          ctx,
          &icon_frame,
          cell->value,
          palette);
      break;
    case GLYPH_KIND_STEPS:
      glyph_lab_draw_steps_icon(
          ctx,
          &icon_frame,
          palette,
          cell->flag);
      break;
    case GLYPH_KIND_BPM:
      glyph_lab_draw_bpm_icon(
          ctx,
          &icon_frame,
          palette,
          s_is_light_mode,
          cell->value,
          cell->flag);
      break;
  }

  draw_cell_label(ctx, &label_frame, palette, cell->label);
}

static void canvas_update_proc(Layer* layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  ColorPalette palette;
  int16_t outer_padding = PBL_IF_ROUND_ELSE(
      OUTER_PADDING_ROUND,
      OUTER_PADDING_RECT);
  GRect content_bounds = grect_inset(
      bounds,
      GEdgeInsets(outer_padding,
                  outer_padding,
                  outer_padding,
                  outer_padding));
  GRect grid_bounds = GRect(
      content_bounds.origin.x,
      content_bounds.origin.y + STATUS_HEIGHT,
      content_bounds.size.w,
      content_bounds.size.h - STATUS_HEIGHT);
  int16_t cell_w = grid_bounds.size.w / GLYPH_COL_COUNT;
  int16_t cell_h = grid_bounds.size.h / GLYPH_ROW_COUNT;
  const GlyphPage* page = &GLYPH_PAGES[s_page_index];

  glyph_lab_select_palette(&palette, s_is_light_mode);
  graphics_context_set_fill_color(ctx, palette.background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_status(ctx, &content_bounds, &palette);

  for (int row = 0; row < GLYPH_ROW_COUNT; ++row) {
    for (int col = 0; col < GLYPH_COL_COUNT; ++col) {
      int index = (row * GLYPH_COL_COUNT) + col;
      GRect cell_frame = GRect(
          grid_bounds.origin.x + (col * cell_w),
          grid_bounds.origin.y + (row * cell_h),
          cell_w,
          cell_h);

      draw_glyph_cell(
          ctx,
          &cell_frame,
          &page->cells[index],
          &palette);
    }
  }
}

static void select_click_handler(
    ClickRecognizerRef recognizer,
    void* ctx) {
  (void)recognizer;
  (void)ctx;

  s_is_light_mode = !s_is_light_mode;
  mark_canvas_dirty();
}

static void up_click_handler(
    ClickRecognizerRef recognizer,
    void* ctx) {
  (void)recognizer;
  (void)ctx;

  s_page_index = (s_page_index + GLYPH_PAGE_COUNT - 1) % GLYPH_PAGE_COUNT;
  mark_canvas_dirty();
}

static void down_click_handler(
    ClickRecognizerRef recognizer,
    void* ctx) {
  (void)recognizer;
  (void)ctx;

  s_page_index = (s_page_index + 1) % GLYPH_PAGE_COUNT;
  mark_canvas_dirty();
}

static void click_config_provider(void* ctx) {
  (void)ctx;

  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(Window* window) {
  Layer* root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_canvas_layer = layer_create(bounds);
  if (!s_canvas_layer) {
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
}

static void init(void) {
  s_status_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_label_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
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
