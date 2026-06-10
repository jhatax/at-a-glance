#include "watchface.h"
#include "background.h"
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

typedef enum {
  NO_STRATA_MASK = 0,
  DATE_STRATUM_MASK = 1,
  TIME_STRATUM_MASK = 2,
  BATTERY_STRATUM_MASK = 4,
  MUST_HAVE_STRATA_MASK = 7,
  CLIMATE_STRATUM_MASK = 8,
  BACKGROUND_STRATUM_MASK = 16,
#ifdef PBL_HEALTH
  BPM_STRATUM_MASK = 32,
  STEPS_STRATUM_MASK = 64
#endif
} StratumMask;
static uint8_t s_strata_created_mask = (uint8_t) NO_STRATA_MASK;

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

  s_strata_created_mask = (uint8_t) NO_STRATA_MASK;

  GRect bounds = layer_get_bounds(root);
  layout_calculate_surface(
      bounds.size.w,
      bounds.size.h,
      s_wf_settings->display_mode,
      &s_surface);

  s_strata_created_mask |= background_module_create(root, &s_surface) ?
      BACKGROUND_STRATUM_MASK : 0;

  s_strata_created_mask |= date_module_create(root, &s_surface) ?
      DATE_STRATUM_MASK : 0;

  s_strata_created_mask |= time_module_create(root, &s_surface) ?
      TIME_STRATUM_MASK : 0;

  s_strata_created_mask |= battery_module_create(root, &s_surface) ?
      BATTERY_STRATUM_MASK : 0;

  if ((s_strata_created_mask & MUST_HAVE_STRATA_MASK) !=
      MUST_HAVE_STRATA_MASK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface must-initialize controls failed");
    watchface_exit_path();
    return false;
  }

  s_strata_created_mask |= climate_module_create(
      root,
      &s_surface,
      s_wf_settings->temp_unit) ? CLIMATE_STRATUM_MASK : 0;

  #ifdef PBL_HEALTH
  s_strata_created_mask |= bpm_module_create(root, &s_surface) ?
      BPM_STRATUM_MASK : 0;

  s_strata_created_mask |= steps_module_create(root, &s_surface) ?
      STEPS_STRATUM_MASK : 0;
  #endif

  return true;
}

void watchface_destroy() {
  if (s_strata_created_mask) {
    if (s_strata_created_mask & BACKGROUND_STRATUM_MASK) {
      background_module_destroy();
      s_strata_created_mask &= ~BACKGROUND_STRATUM_MASK;
    }

    if (s_strata_created_mask & DATE_STRATUM_MASK) {
      date_module_destroy();
      s_strata_created_mask &= ~DATE_STRATUM_MASK;
    }

    if (s_strata_created_mask & TIME_STRATUM_MASK) {
      time_module_destroy();
      s_strata_created_mask &= ~TIME_STRATUM_MASK;
    }

    if (s_strata_created_mask & BATTERY_STRATUM_MASK) {
      battery_module_destroy();
      s_strata_created_mask &= ~BATTERY_STRATUM_MASK;
    }

    if (s_strata_created_mask & CLIMATE_STRATUM_MASK) {
      climate_module_destroy();
      s_strata_created_mask &= ~CLIMATE_STRATUM_MASK;
    }

    #ifdef PBL_HEALTH
    if (s_strata_created_mask & BPM_STRATUM_MASK) {
      bpm_module_destroy();
      s_strata_created_mask &= ~BPM_STRATUM_MASK;
    }

    if (s_strata_created_mask & STEPS_STRATUM_MASK) {
      steps_module_destroy();
      s_strata_created_mask &= ~STEPS_STRATUM_MASK;
    }
    #endif
  }

  s_strata_created_mask = (uint8_t) NO_STRATA_MASK;
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
  background_module_refresh(&s_surface);

  date_module_refresh(&s_surface);
  time_module_refresh(&s_surface, s_wf_settings->time_format);
  battery_module_refresh(&s_surface);

  if (s_strata_created_mask & CLIMATE_STRATUM_MASK) {
    climate_module_refresh(&s_surface, s_wf_settings->temp_unit);
  }

  #ifdef PBL_HEALTH
  if (s_strata_created_mask & BPM_STRATUM_MASK) {
    bpm_module_refresh(&s_surface);
  }

  if (s_strata_created_mask & STEPS_STRATUM_MASK) {
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

  if (s_strata_created_mask & BPM_STRATUM_MASK) {
    bpm_module_handle_event(event);
  }

  if (s_strata_created_mask & STEPS_STRATUM_MASK) {
    steps_module_handle_event(event);
  }
}

#ifdef DEBUG_ATAGLANCE
void watchface_debug_update_bpm(int bpm) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();

  if (s_strata_created_mask & BPM_STRATUM_MASK) {
    bpm_module_debug_set_value(bpm);
  }
}

void watchface_debug_update_steps(int steps) {
  if (!s_wf_settings || !s_wf_window) {
    return;
  }

  watchface_update_style();

  if (s_strata_created_mask & STEPS_STRATUM_MASK) {
    steps_module_debug_set_value(steps);
  }
}
#endif
#endif
