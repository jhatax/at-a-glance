#include "glyph_lab_glyphs.h"
#include "glyph_lab_helper.h"

#define GLYPH_COL_COUNT 2
#define GLYPH_ROW_COUNT 2
#define STATUS_HEIGHT 18
#define LABEL_HEIGHT 14
#define OUTER_PADDING_RECT 6
#define OUTER_PADDING_ROUND 12
#define CELL_PADDING 4

typedef struct {
  const char* label;
  int weather_code;
  bool is_day;
} ClimateCell;

typedef struct {
  const char* title;
  ClimateCell cells[GLYPH_COL_COUNT * GLYPH_ROW_COUNT];
} ClimatePageTemplate;

typedef struct {
  const char* title;
  int16_t icon_size;
} GlyphSizeMode;

static Window* s_window;
static Layer* s_canvas_layer;
static GFont s_status_font;
static GFont s_label_font;
static uint8_t s_palette_index = 0;
static const char* const PALETTE_NAMES[] = {"DARK", "LIGHT", "DARK HC", "LIGHT HC"};
static int s_page_index = 0;

static const ClimatePageTemplate GLYPH_PAGE_TEMPLATES[] = {
    {
        .title = "CLEAR CLOUD",
        .cells =
            {
                {"CLEAR D", 0, true},
                {"CLEAR N", 0, false},
                {"PARTLY CLOUDY", 2, true},
                {"CLOUD", 3, false},
            },
    },
    {
        .title = "FOG DRIZZLE",
        .cells =
            {
                {"FOG", 45, false},
                {"DRIZZLE", 51, false},
                {"RAIN", 61, false},
                {"HEAVY", 65, false},
            },
    },
    {
        .title = "SLEET SNOW",
        .cells =
            {
                {"SLEET L", 56, false},
                {"SLEET H", 66, false},
                {"SNOW", 71, false},
                {"SNOW SH", 85, false},
            },
    },
    {
        .title = "SHOWERS ETC",
        .cells =
            {
                {"SHOWERS", 80, false},
                {"HEAVY SH", 82, false},
                {"STORM", 95, false},
                {"OUT OF RANGE", -1, false},
            },
    },
    {
        .title = "TROUBLE MAKERS",
        .cells =
            {
                {"PARTLY CLOUDY", 2, false},
                {"SLEET H", 66, false},
                {"SNOW", 71, false},
                {"SNOW SH", 85, false},
            },
    },
};

static const GlyphSizeMode GLYPH_SIZE_MODES[] = {
    {
        .title = "HAND 28",
        .icon_size = 28,
    },
    {
        .title = "HAND 18",
        .icon_size = 18,
    },
};

static const int GLYPH_PAGE_TEMPLATE_COUNT =
    sizeof(GLYPH_PAGE_TEMPLATES) / sizeof(GLYPH_PAGE_TEMPLATES[0]);

static const int GLYPH_PAGE_COUNT =
    (sizeof(GLYPH_PAGE_TEMPLATES) / sizeof(GLYPH_PAGE_TEMPLATES[0])) *
    (sizeof(GLYPH_SIZE_MODES) / sizeof(GLYPH_SIZE_MODES[0]));

static void mark_canvas_dirty() {
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void draw_status(
    GContext* ctx,
    const GRect* bounds,
    const ColorPalette* palette) {
  char status[40];
  int size_index = s_page_index / GLYPH_PAGE_TEMPLATE_COUNT;
  int template_index = s_page_index % GLYPH_PAGE_TEMPLATE_COUNT;
  const GlyphSizeMode* size_mode = &GLYPH_SIZE_MODES[size_index];
  const ClimatePageTemplate* page_template = &GLYPH_PAGE_TEMPLATES[template_index];

  snprintf(status,
      sizeof(status),
      "%d/%d %s %s %s",
      s_page_index + 1,
      GLYPH_PAGE_COUNT,
      size_mode->title,
      page_template->title,
      PALETTE_NAMES[s_palette_index]);

  graphics_context_set_text_color(ctx, palette->primary_text);
  graphics_draw_text(ctx,
      status,
      s_status_font,
      GRect(bounds->origin.x, bounds->origin.y, bounds->size.w, STATUS_HEIGHT),
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter,
      NULL);
}

static void draw_cell_label(
    GContext* ctx,
    const GRect* frame,
    const ColorPalette* palette,
    const char* label) {
  graphics_context_set_text_color(ctx, palette->outofrange_text);
  graphics_draw_text(ctx,
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
    const ClimateCell* cell,
    const ColorPalette* palette,
    const GlyphSizeMode* size_mode) {
  GRect icon_area = GRect(cell_frame->origin.x + CELL_PADDING,
      cell_frame->origin.y + CELL_PADDING,
      cell_frame->size.w - (CELL_PADDING * 2),
      cell_frame->size.h - LABEL_HEIGHT - (CELL_PADDING * 2));
  int16_t icon_size = HELPER_MIN(icon_area.size.w, icon_area.size.h);

  if (size_mode->icon_size > 0 && size_mode->icon_size < icon_size) {
    icon_size = size_mode->icon_size;
  }

  GRect icon_frame = GRect(icon_area.origin.x + ((icon_area.size.w - icon_size) / 2),
      icon_area.origin.y + ((icon_area.size.h - icon_size) / 2),
      icon_size,
      icon_size);
  GRect label_frame = GRect(cell_frame->origin.x + 2,
      cell_frame->origin.y + cell_frame->size.h - LABEL_HEIGHT,
      cell_frame->size.w - 4,
      LABEL_HEIGHT);

  glyph_lab_draw_climate_icon(ctx, &icon_frame, cell->weather_code, cell->is_day, palette);
  draw_cell_label(ctx, &label_frame, palette, cell->label);
}

static void canvas_update_proc(
    Layer* layer,
    GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  ColorPalette palette;
  int size_index = s_page_index / GLYPH_PAGE_TEMPLATE_COUNT;
  int template_index = s_page_index % GLYPH_PAGE_TEMPLATE_COUNT;
  int16_t outer_padding = PBL_IF_ROUND_ELSE(OUTER_PADDING_ROUND, OUTER_PADDING_RECT);
  GRect content_bounds =
      grect_inset(bounds, GEdgeInsets(outer_padding, outer_padding, outer_padding, outer_padding));
  GRect grid_bounds = GRect(content_bounds.origin.x,
      content_bounds.origin.y + STATUS_HEIGHT,
      content_bounds.size.w,
      content_bounds.size.h - STATUS_HEIGHT);
  int16_t cell_w = grid_bounds.size.w / GLYPH_COL_COUNT;
  int16_t cell_h = grid_bounds.size.h / GLYPH_ROW_COUNT;
  const ClimatePageTemplate* page_template = &GLYPH_PAGE_TEMPLATES[template_index];
  const GlyphSizeMode* size_mode = &GLYPH_SIZE_MODES[size_index];

  glyph_lab_select_palette(&palette, s_palette_index);
  graphics_context_set_fill_color(ctx, palette.background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_status(ctx, &content_bounds, &palette);

  for (uint8_t row = 0; row < GLYPH_ROW_COUNT; ++row) {
    for (uint8_t col = 0; col < GLYPH_COL_COUNT; ++col) {
      int index = (row * GLYPH_COL_COUNT) + col;
      GRect cell_frame = GRect(grid_bounds.origin.x + (col * cell_w),
          grid_bounds.origin.y + (row * cell_h),
          cell_w,
          cell_h);

      draw_glyph_cell(ctx, &cell_frame, &page_template->cells[index], &palette, size_mode);
    }
  }
}

static void select_click_handler(
    ClickRecognizerRef recognizer,
    void* ctx) {
  (void)recognizer;
  (void)ctx;

  s_palette_index = (s_palette_index + 1) % ARRAY_LENGTH(PALETTE_NAMES);
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

static void click_config_provider(
    void* ctx) {
  (void)ctx;

  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(
    Window* window) {
  Layer* root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_canvas_layer = layer_create(bounds);
  if (!s_canvas_layer) {
    return;
  }

  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);
}

static void main_window_unload(
    Window* window) {
  (void)window;

  if (s_canvas_layer) {
    layer_destroy(s_canvas_layer);
    s_canvas_layer = NULL;
  }
}

static void init() {
  s_status_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_label_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  glyph_lab_glyphs_init();
  s_window = window_create();
  if (!s_window) {
    return;
  }

  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window,
      (WindowHandlers){
          .load = main_window_load,
          .unload = main_window_unload,
      });
  window_stack_push(s_window, true);
}

static void deinit() {
  glyph_lab_glyphs_deinit();
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}

int main() {
  init();
  app_event_loop();
  deinit();
}
