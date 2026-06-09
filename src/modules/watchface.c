#include "watchface.h"
#include "battery.h"
#include "climate.h"
#include "date.h"
#include "layout.h"
#include "time.h"

#ifdef PBL_HEALTH
#include "bpm.h"
#include "steps.h"
#endif

static WatchfaceSurface s_surface = {0};
static Window* s_wf_window = NULL;
static const WatchfaceSettings* s_wf_settings = NULL;
static Layer* s_background_layer = NULL;

typedef enum {
  NO_LAYERS_MASK = 0,
  DATE_LAYER_MASK = 1,
  TIME_LAYER_MASK = 2,
  BATTERY_LAYER_MASK = 4,
  MUST_HAVE_LAYERS_MASK = 7,
  CLIMATE_LAYER_MASK = 8,
#ifdef PBL_HEALTH
  BPM_LAYER_MASK = 16,
  STEPS_LAYER_MASK = 32,
  ALL_LAYERS_MASK = 63
#else
  ALL_LAYERS_MASK = 15
#endif
} LayerMask;
static uint8_t s_layers_created = (uint8_t) NO_LAYERS_MASK;

static void background_layer_update_proc(Layer* layer, GContext* ctx) {
  if (!layer || !ctx || !s_surface.style.palette) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(
      ctx,
      s_surface.style.palette->background_layer_background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (!s_surface.background.line_enabled) {
    return;
  }

  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(
      ctx,
      s_surface.style.palette->background_layer_line);
  graphics_draw_line(
      ctx,
      GPoint(s_surface.background.line_x, s_surface.background.line_y),
      GPoint(s_surface.background.line_x + s_surface.background.line_width,
             s_surface.background.line_y));
}

static void watchface_exit_path() {
  watchface_destroy();
  window_stack_pop_all(false);
}

static void watchface_update_style(void) {
  if (!s_wf_settings) {
    return;
  }

  layout_update_surface_style(&s_surface, s_wf_settings->display_mode);
}

bool watchface_create(
    Window* window,
    const WatchfaceSettings* settings) {
  if (!window || !settings) {
    APP_LOG(APP_LOG_LEVEL_ERROR,
            "Cannot create watchface without watchface inputs");
    if (window) {
      watchface_exit_path();
    }
    return false;
  }

  s_wf_window = window;
  s_wf_settings = settings;

  Layer* root = window_get_root_layer(window);

  if (!root) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window root layer is NULL");
    watchface_exit_path();
    return false;
  }

  s_layers_created = (uint8_t) NO_LAYERS_MASK;

  GRect bounds = layer_get_bounds(root);
  layout_calculate_surface(
      bounds.size.w,
      bounds.size.h,
      s_wf_settings->display_mode,
      &s_surface);

  s_background_layer = layer_create(s_surface.background.frame);
  if (s_background_layer) {
    layer_set_update_proc(s_background_layer, background_layer_update_proc);
    layer_add_child(root, s_background_layer);
  }

  s_layers_created |= date_module_create(root, &s_surface) ?
      DATE_LAYER_MASK : 0;

  s_layers_created |= time_module_create(root, &s_surface) ?
      TIME_LAYER_MASK : 0;

  s_layers_created |= battery_module_create(root, &s_surface) ?
      BATTERY_LAYER_MASK : 0;

  if ((s_layers_created & MUST_HAVE_LAYERS_MASK) != MUST_HAVE_LAYERS_MASK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface must-initialize controls failed");
    watchface_exit_path();
    return false;
  }

  s_layers_created |= climate_module_create(
      root,
      &s_surface,
      s_wf_settings->temp_unit) ? CLIMATE_LAYER_MASK : 0;

  #ifdef PBL_HEALTH
  s_layers_created |= bpm_module_create(root, &s_surface) ?
      BPM_LAYER_MASK : 0;

  s_layers_created |= steps_module_create(root, &s_surface) ?
      STEPS_LAYER_MASK : 0;
  #endif

  return true;
}

void watchface_destroy() {
  if (s_layers_created) {
    if (s_background_layer) {
      layer_destroy(s_background_layer);
      s_background_layer = NULL;
    }

    if (s_layers_created & DATE_LAYER_MASK) {
      date_module_destroy();
      s_layers_created &= ~DATE_LAYER_MASK;
    }

    if (s_layers_created & TIME_LAYER_MASK) {
      time_module_destroy();
      s_layers_created &= ~TIME_LAYER_MASK;
    }

    if (s_layers_created & BATTERY_LAYER_MASK) {
      battery_module_destroy();
      s_layers_created &= ~BATTERY_LAYER_MASK;
    }

    if (s_layers_created & CLIMATE_LAYER_MASK) {
      climate_module_destroy();
      s_layers_created &= ~CLIMATE_LAYER_MASK;
    }

    #ifdef PBL_HEALTH
    if (s_layers_created & BPM_LAYER_MASK) {
      bpm_module_destroy();
      s_layers_created &= ~BPM_LAYER_MASK;
    }

    if (s_layers_created & STEPS_LAYER_MASK) {
      steps_module_destroy();
      s_layers_created &= ~STEPS_LAYER_MASK;
    }
    #endif
  }

  s_layers_created = (uint8_t) NO_LAYERS_MASK;
  if (s_background_layer) {
    layer_destroy(s_background_layer);
    s_background_layer = NULL;
  }
  s_wf_settings = NULL;
  s_wf_window = NULL;
  memset(&s_surface, 0, sizeof(s_surface));
}

void watchface_refresh() {
  if (!s_wf_window || !s_wf_settings) {
    return;
  }

  watchface_update_style();
  window_set_background_color(
      s_wf_window,
      s_surface.style.palette->background);
  if (s_background_layer) {
    layer_mark_dirty(s_background_layer);
  }

  date_module_refresh(&s_surface);
  time_module_refresh(&s_surface, s_wf_settings->time_format);
  battery_module_refresh(&s_surface);

  if (s_layers_created & CLIMATE_LAYER_MASK) {
    climate_module_refresh(&s_surface, s_wf_settings->temp_unit);
  }

  #ifdef PBL_HEALTH
  if (s_layers_created & BPM_LAYER_MASK) {
    bpm_module_refresh(&s_surface);
  }

  if (s_layers_created & STEPS_LAYER_MASK) {
    steps_module_refresh(&s_surface);
  }
  #endif
}

void watchface_handle_tick(TimeUnits units_changed) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();

  if (units_changed & MINUTE_UNIT) {
    time_module_refresh(&s_surface, s_wf_settings->time_format);
  }
  if (units_changed & DAY_UNIT) {
    date_module_refresh(&s_surface);
  }
}

void watchface_update_battery(const BatteryChargeState* state) {
  if (!s_wf_settings || !s_wf_window || !state) {
    return;
  }

  watchface_update_style();
  battery_module_set_state(state);
}

void watchface_update_temp(int celsius_tenths, uint8_t temp_unit) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();
  climate_module_set_temperature(celsius_tenths, temp_unit, &s_surface);
}

void watchface_update_weather_condition(
    int weather_condition,
    uint8_t temp_unit) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();
  climate_module_set_condition(weather_condition, temp_unit, &s_surface);
}

#if defined(PBL_HEALTH)
void watchface_handle_health_event(HealthEventType event) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();

  if (s_layers_created & BPM_LAYER_MASK) {
    bpm_module_handle_event(event);
  }

  if (s_layers_created & STEPS_LAYER_MASK) {
    steps_module_handle_event(event);
  }
}

#ifdef DEBUG_ATAGLANCE
void watchface_debug_update_bpm(int bpm) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();

  if (s_layers_created & BPM_LAYER_MASK) {
    bpm_module_debug_set_value(bpm);
  }
}

void watchface_debug_update_steps(int steps) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();

  if (s_layers_created & STEPS_LAYER_MASK) {
    steps_module_debug_set_value(steps);
  }
}
#endif
#endif
