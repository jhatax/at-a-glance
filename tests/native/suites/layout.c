#include <math.h>
#include <stdlib.h>

#include "modules/substratum_computations.h"
#include "modules/watchface_layout.h"
#include "test_runner.h"

// Suite: calculated frame containment, battery relayout, and coordinate scaling.
// Frames hold references to calculated geometry, never expected coordinates.
typedef enum { CONTENT_RECTANGLE, CONTENT_FRAME, CONTENT_CIRCLE } ContentShape;

// Clar lifecycle: none; each case builds its inputs in local variables.

// Suite helpers: called explicitly by test cases.
static bool contains(
    const GRect* outer,
    const GRect* inner) {
  return inner->size.w > 0 && inner->size.h > 0 && inner->origin.x >= outer->origin.x &&
         inner->origin.y >= outer->origin.y &&
         inner->origin.x + inner->size.w <= outer->origin.x + outer->size.w &&
         inner->origin.y + inner->size.h <= outer->origin.y + outer->size.h;
}

// All circular comparisons use doubled continuous coordinates: no rounding or sqrt.
static bool rectangle_in_circle(
    const GRect* rect,
    int width,
    int height) {
  const int radius = HELPER_MIN(width, height);
  const int xs[] = {2 * rect->origin.x - width, 2 * (rect->origin.x + rect->size.w) - width};
  const int ys[] = {2 * rect->origin.y - height, 2 * (rect->origin.y + rect->size.h) - height};
  for (size_t x = 0; x < 2; ++x)
    for (size_t y = 0; y < 2; ++y)
      if (xs[x] * xs[x] + ys[y] * ys[y] > radius * radius) return false;
  return true;
}

static bool rectangle_intersects_circle(
    const GRect* rect,
    int width,
    int height) {
  const int radius = HELPER_MIN(width, height);
  const int left = 2 * rect->origin.x - width;
  const int right = 2 * (rect->origin.x + rect->size.w) - width;
  const int top = 2 * rect->origin.y - height;
  const int bottom = 2 * (rect->origin.y + rect->size.h) - height;
  const int x = left > 0 ? left : (right < 0 ? right : 0);
  const int y = top > 0 ? top : (bottom < 0 ? bottom : 0);
  return x * x + y * y <= radius * radius;
}

static bool circle_in_circle(
    const GRect* rect,
    int width,
    int height) {
  // GOvalScaleModeFitCircle uses the smaller dimension of its enclosing frame.
  const int radius = HELPER_MIN(width, height);
  const int inner_radius = HELPER_MIN(rect->size.w, rect->size.h);
  const int dx = 2 * rect->origin.x + rect->size.w - width;
  const int dy = 2 * rect->origin.y + rect->size.h - height;
  const int room = radius - inner_radius;
  return room >= 0 && dx * dx + dy * dy <= room * room;
}

static int check_frame(
    const char* name,
    const GRect* frame,
    ContentShape shape,
    bool vertical) {
  const GRect screen = GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT);
  const char* orientation = vertical ? "vertical" : "horizontal";
  if (getenv("NATIVE_LAYOUT_BOUNDS")) {
    printf(
        "BOUNDS %s %s: origin (%d, %d) | width %d | height %d | bottom-right (%d, %d)\n",
        orientation,
        name,
        frame->origin.x,
        frame->origin.y,
        frame->size.w,
        frame->size.h,
        frame->origin.x + frame->size.w,
        frame->origin.y + frame->size.h);
  }
  if (!contains(&screen, frame)) {
    fflush(stdout);
    fprintf(
        stderr,
        "BOUNDARY VIOLATION %s %s: origin (%d, %d) | width %d | height %d | "
        "bottom-right (%d, %d); screen=%dx%d; "
        "overflow left=%d top=%d right=%d bottom=%d pixels; positive_size=%s\n",
        orientation,
        name,
        frame->origin.x,
        frame->origin.y,
        frame->size.w,
        frame->size.h,
        frame->origin.x + frame->size.w,
        frame->origin.y + frame->size.h,
        PBL_DISPLAY_WIDTH,
        PBL_DISPLAY_HEIGHT,
        HELPER_MAX(0, -frame->origin.x),
        HELPER_MAX(0, -frame->origin.y),
        HELPER_MAX(0, frame->origin.x + frame->size.w - PBL_DISPLAY_WIDTH),
        HELPER_MAX(0, frame->origin.y + frame->size.h - PBL_DISPLAY_HEIGHT),
        frame->size.w > 0 && frame->size.h > 0 ? "yes" : "no");
    return 1;
  }
#ifdef PBL_ROUND
  bool fits = shape == CONTENT_CIRCLE
                  ? circle_in_circle(frame, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT)
                  : rectangle_in_circle(frame, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT);
  if (!fits) {
    // A text/image frame can contain unused corners. Its intersection is a
    // mathematical observation; the enclosed content's footprint is unknown here.
    bool violation = shape != CONTENT_FRAME ||
                     !rectangle_intersects_circle(frame, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT);
    if (!violation && !getenv("NATIVE_LAYOUT_BOUNDS")) return 0;
    const int radius = HELPER_MIN(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT);
    double excess;
    if (shape == CONTENT_CIRCLE) {
      const int dx = 2 * frame->origin.x + frame->size.w - PBL_DISPLAY_WIDTH;
      const int dy = 2 * frame->origin.y + frame->size.h - PBL_DISPLAY_HEIGHT;
      excess = (hypot(dx, dy) + HELPER_MIN(frame->size.w, frame->size.h) - radius) / 2.0;
    } else {
      const int dx = HELPER_MAX(
          abs(2 * frame->origin.x - PBL_DISPLAY_WIDTH),
          abs(2 * (frame->origin.x + frame->size.w) - PBL_DISPLAY_WIDTH));
      const int dy = HELPER_MAX(
          abs(2 * frame->origin.y - PBL_DISPLAY_HEIGHT),
          abs(2 * (frame->origin.y + frame->size.h) - PBL_DISPLAY_HEIGHT));
      excess = (hypot(dx, dy) - radius) / 2.0;
    }
    if (violation) fflush(stdout);
    fprintf(
        violation ? stderr : stdout,
        "%s %s %s: origin (%d, %d) | width %d | height %d | bottom-right (%d, %d); "
        "screen_circle=%dx%d; "
        "%s radial overflow=%.3f pixels",
        violation ? "CIRCLE VIOLATION" : "FRAME CIRCLE INTERSECTION",
        orientation,
        name,
        frame->origin.x,
        frame->origin.y,
        frame->size.w,
        frame->size.h,
        frame->origin.x + frame->size.w,
        frame->origin.y + frame->size.h,
        PBL_DISPLAY_WIDTH,
        PBL_DISPLAY_HEIGHT,
        shape == CONTENT_FRAME ? "frame" : "shape",
        excess);
    fprintf(violation ? stderr : stdout, "\n");
    return violation;
  }
#else
  (void)shape;
#endif
  return 0;
}

static int check_surface(
    const WatchfaceSurface* surface) {
  ContentShape battery_shape = CONTENT_RECTANGLE;
#ifdef PBL_ROUND
  if (surface->battery.is_vertical) battery_shape = CONTENT_CIRCLE;
#endif
  struct Frame {
    const char* name;
    const GRect* rect;
    ContentShape shape;
  } frames[] = {
      {"time", &surface->time.frame, CONTENT_FRAME},
      {"date", &surface->date.frame, CONTENT_FRAME},
      {"location", &surface->location.frame, CONTENT_FRAME},
      {"climate.icon", &surface->climate.icon, CONTENT_FRAME},
      {"climate.text", &surface->climate.text.frame, CONTENT_FRAME},
      {"battery.track", &surface->battery.track, battery_shape},
      {"battery.fill", &surface->battery.fill, battery_shape},
      {"battery.bolt", &surface->battery.bolt, CONTENT_FRAME},
      {"bluetooth", &surface->bt_icon, CONTENT_FRAME},
      {"horizontal_rule", &surface->horiz_rule, CONTENT_RECTANGLE},
#ifdef PBL_HEALTH
      {"steps.icon", &surface->steps.icon, CONTENT_FRAME},
      {"steps.text", &surface->steps.text.frame, CONTENT_FRAME},
      {"steps.progress", &surface->steps.progress, CONTENT_RECTANGLE},
      {"bpm.icon", &surface->bpm.icon, CONTENT_FRAME},
      {"bpm.text", &surface->bpm.text.frame, CONTENT_FRAME},
#endif
  };
  // Stable insertion sort: origin Y first, then X; preserve catalogue order for ties.
  for (size_t i = 1; i < ARRAY_LENGTH(frames); ++i) {
    const struct Frame frame = frames[i];
    size_t j = i;
    while (j > 0 && (frames[j - 1].rect->origin.y > frame.rect->origin.y ||
                     (frames[j - 1].rect->origin.y == frame.rect->origin.y &&
                      frames[j - 1].rect->origin.x > frame.rect->origin.x))) {
      frames[j] = frames[j - 1];
      --j;
    }
    frames[j] = frame;
  }
  const bool show_bounds = getenv("NATIVE_LAYOUT_BOUNDS") != NULL;
  if (show_bounds) {
    printf(
        "Geometry: %s / %s / screen=%dx%d; origin Y ascending, then X\n",
        TEST_PLATFORM,
        surface->battery.is_vertical ? "vertical" : "horizontal",
        PBL_DISPLAY_WIDTH,
        PBL_DISPLAY_HEIGHT);
    printf("Top-Left: (0, 0) | Top-Right: (%d, 0)\n", PBL_DISPLAY_WIDTH);
  }
  int violations = 0;
  for (size_t i = 0; i < ARRAY_LENGTH(frames); ++i)
    violations +=
        check_frame(frames[i].name, frames[i].rect, frames[i].shape, surface->battery.is_vertical);

  if (show_bounds) {
    printf(
        "Bottom-Left: (0, %d) | Bottom-Right: (%d, %d)\n",
        PBL_DISPLAY_HEIGHT,
        PBL_DISPLAY_WIDTH,
        PBL_DISPLAY_HEIGHT);
  }
  fflush(stdout);
  cl_check_(contains(&surface->battery.track, &surface->battery.fill), "battery fill containment");
  cl_check_(
      surface->climate.icon.origin.x + surface->climate.icon.size.w <=
          surface->climate.text.frame.origin.x,
      "climate icon/text separation");
  cl_check_(
      surface->climate.text.frame.origin.x + surface->climate.text.frame.size.w <=
          surface->date.frame.origin.x,
      "climate/date separation");
#ifdef PBL_HEALTH
  cl_check_(
      surface->steps.icon.origin.x + surface->steps.icon.size.w <=
          surface->steps.text.frame.origin.x,
      "steps icon/text separation");
  cl_check_(
      surface->bpm.icon.origin.x + surface->bpm.icon.size.w <= surface->bpm.text.frame.origin.x,
      "bpm icon/text separation");
#endif
  if (show_bounds) {
    printf(
        "Geometry: %s, %zu elements, %d boundary violations\n",
        surface->battery.is_vertical ? "vertical" : "horizontal",
        ARRAY_LENGTH(frames),
        violations);
  }
  return violations;
}

// Test cases.
void test_layout__containment_rules(
    void) {
  const GRect screen = GRect(0, 0, 100, 100);
  const GRect exact = GRect(0, 0, 100, 100);
  const GRect center = GRect(40, 40, 20, 20);
  const GRect corner = GRect(0, 0, 2, 2);
  const GRect shifted = GRect(1, 0, 100, 100);
  const GRect outside[] = {
      GRect(-1, 0, 1, 1),
      GRect(0, -1, 1, 1),
      GRect(99, 0, 2, 1),
      GRect(0, 99, 1, 2),
      GRect(0, 0, 0, 1),
      GRect(0, 0, 1, -1)};
  cl_assert(contains(&screen, &exact));
  for (size_t i = 0; i < ARRAY_LENGTH(outside); ++i) {
    test_context("containment boundary row=%zu", i);
    cl_assert(!contains(&screen, &outside[i]));
  }
  cl_assert(rectangle_in_circle(&center, 100, 100));
  cl_assert(!rectangle_in_circle(&exact, 100, 100));
  cl_assert(rectangle_intersects_circle(&exact, 100, 100));
  cl_assert(!rectangle_intersects_circle(&corner, 100, 100));
  cl_assert(circle_in_circle(&exact, 100, 100));
  cl_assert(!circle_in_circle(&shifted, 100, 100));
}

void test_layout__screen_bounds(
    void) {
  cl_assert(!layout_watchface_prepare(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT, false, NULL));
  relayout_battery_bolt_bticon(NULL);
  int violations = 0;
  for (int vertical = 0; vertical <= 1; ++vertical) {
    test_context("battery orientation=%s", vertical ? "vertical" : "horizontal");
    WatchfaceSurface surface;
    memset(&surface, 0xA5, sizeof(surface));
    cl_assert(layout_watchface_prepare(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT, vertical, &surface));
    cl_assert_equal_i(PBL_DISPLAY_WIDTH, surface.face_width);
    cl_assert_equal_i(PBL_DISPLAY_HEIGHT, surface.face_height);
    cl_assert_equal_i(vertical, surface.battery.is_vertical);
    cl_assert_equal_p(NULL, surface.style.palette);
    violations += check_surface(&surface);
  }
  cl_assert_equal_i(0, violations);
}

void test_layout__orientation_round_trip(
    void) {
  WatchfaceSurface initial, changed, prepared;
  cl_assert(layout_watchface_prepare(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT, false, &initial));
  changed = initial;
  changed.battery.is_vertical = true;
  relayout_battery_bolt_bticon(&changed);
  cl_assert(layout_watchface_prepare(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT, true, &prepared));
  cl_assert(memcmp(&changed, &prepared, sizeof(changed)) == 0);
  changed.battery.is_vertical = false;
  relayout_battery_bolt_bticon(&changed);
  cl_assert(memcmp(&changed, &initial, sizeof(changed)) == 0);
}

void test_layout__scaled_coordinates(
    void) {
  // Every reference-grid point must stay within each placed icon frame.
  for (int vertical = 0; vertical <= 1; ++vertical) {
    WatchfaceSurface surface;
    cl_assert(layout_watchface_prepare(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT, vertical, &surface));
    const GRect frames[] = {
        surface.climate.icon,
        surface.battery.bolt,
        surface.bt_icon,
#ifdef PBL_HEALTH
        surface.steps.icon,
        surface.bpm.icon,
#endif
    };
    for (size_t i = 0; i < ARRAY_LENGTH(frames); ++i) {
      for (int16_t x = 0; x < WATCHFACE_ICON_GRID_WIDTH; ++x) {
        for (int16_t y = 0; y < WATCHFACE_ICON_GRID_HEIGHT; ++y) {
          GPoint point = substratum_renderer_scale_icon_x_y_in_frame(&frames[i], x, y);
          cl_assert(point.x >= frames[i].origin.x && point.y >= frames[i].origin.y);
          cl_assert(point.x < frames[i].origin.x + frames[i].size.w);
          cl_assert(point.y < frames[i].origin.y + frames[i].size.h);
        }
      }
      GRect subframe;
      substratum_renderer_create_subframe(
          &frames[i],
          &subframe,
          0,
          0,
          WATCHFACE_ICON_GRID_WIDTH,
          WATCHFACE_ICON_GRID_HEIGHT);
      cl_assert(contains(&frames[i], &subframe));
      cl_assert(subframe.size.w == frames[i].size.w && subframe.size.h == frames[i].size.h);
    }
  }
}
