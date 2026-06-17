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

// A consequence of this is that s_surface.is_compact = false by default
static WatchfaceSurface s_surface = {0};
static GFont s_custom_fonts[WATCHFACE_FONT_ROLE_COUNT] = {0};
static Window* s_wf_window = NULL;
static const WatchfaceSettings* s_wf_settings = NULL;
static bool s_watchface_initialized = false;
static const WatchfaceUpdateMask WATCHFACE_UPDATE_ALL_STRATA =
    WATCHFACE_UPDATE_TIME |
    WATCHFACE_UPDATE_DATE |
    WATCHFACE_UPDATE_BATTERY |
    WATCHFACE_UPDATE_CLIMATE
#ifdef PBL_HEALTH
    | WATCHFACE_UPDATE_HEALTH
#endif
    ;

static void watchface_refresh_strata(WatchfaceUpdateMask updates);

typedef enum {
  NO_STRATA_MASK = 0,
  DATE_STRATUM_MASK = 1,
  TIME_STRATUM_MASK = 2,
  BATTERY_STRATUM_MASK = 4,
  MUST_HAVE_STRATA_MASK = 7,
  CLIMATE_STRATUM_MASK = 8,
#ifdef PBL_HEALTH
  BPM_STRATUM_MASK = 16,
  STEPS_STRATUM_MASK = 32
#endif
} StratumMask;
static uint8_t s_strata_created_mask = (uint8_t) NO_STRATA_MASK;

static void watchface_apply_custom_fonts(void);
static bool watchface_load_custom_fonts(void);
static void watchface_unload_custom_fonts(void);
static void watchface_exit_path() {
  watchface_destroy();
  window_stack_pop_all(false);
}

static GFont watchface_find_loaded_custom_font(
    uint8_t max_role,
    uint32_t resource_id) {
  for (uint8_t i = 0; i < max_role; ++i) {
    if (s_surface.style.custom_font_resource_ids[i] == resource_id &&
        s_custom_fonts[i]) {
      return s_custom_fonts[i];
    }
  }

  return NULL;
}

static void watchface_apply_custom_fonts(void) {
  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    if (s_custom_fonts[i]) {
      s_surface.style.fonts[i] = s_custom_fonts[i];
    }
  }
}

static bool watchface_load_custom_fonts(void) {
  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    const uint32_t resource_id = s_surface.style.custom_font_resource_ids[i];
    if (!resource_id) {
      continue;
    }

    GFont shared_font = watchface_find_loaded_custom_font(i, resource_id);
    if (shared_font) {
      s_custom_fonts[i] = shared_font;
      continue;
    }

    s_custom_fonts[i] = fonts_load_custom_font(resource_get_handle(resource_id));
    if (!s_custom_fonts[i]) {
      APP_LOG(APP_LOG_LEVEL_ERROR,
              "Failed to load watchface custom font resource %lu",
              (unsigned long) resource_id);
      watchface_unload_custom_fonts();
      return false;
    }
  }

  watchface_apply_custom_fonts();
  return true;
}

static void watchface_unload_custom_fonts(void) {
  for (uint8_t i = 0; i < WATCHFACE_FONT_ROLE_COUNT; ++i) {
    const uint32_t resource_id = s_surface.style.custom_font_resource_ids[i];
    if (s_custom_fonts[i] &&
        resource_id &&
        !watchface_find_loaded_custom_font(i, resource_id)) {
      fonts_unload_custom_font(s_custom_fonts[i]);
    }
    s_custom_fonts[i] = NULL;
  }
}

bool watchface_create(Window* window, const WatchfaceSettings* settings) {
  if (s_watchface_initialized) {
    return true;
  }

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
  if (!layout_watchface_initialize(
      bounds.size.w,
      bounds.size.h,
      &s_surface)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface layout initialization failed");
    watchface_exit_path();
    return false;
  }

  // Update the style
  layout_update_watchface_style(
      &(s_surface.style),
      s_wf_settings->display_mode);
  if (!watchface_load_custom_fonts()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface custom font initialization failed");
    watchface_exit_path();
    return false;
  }

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

  s_watchface_initialized = true;

  if (s_surface.style.palette) {
    window_set_background_color(
        s_wf_window,
        s_surface.style.palette->background);
  }
  watchface_refresh_strata(WATCHFACE_UPDATE_ALL_STRATA);
  return true;
}

void watchface_destroy() {
  if (s_strata_created_mask) {
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

  watchface_unload_custom_fonts();

  s_strata_created_mask = (uint8_t) NO_STRATA_MASK;
  s_wf_settings = NULL;
  s_wf_window = NULL;
  s_watchface_initialized = false;
  memset(&s_surface, 0, sizeof(s_surface));
}

void watchface_repaint(void) {
  if (!s_wf_window ||
      !s_wf_settings) {
    return;
  }

  layout_update_watchface_style(
      &(s_surface.style),
      s_wf_settings->display_mode);
  watchface_apply_custom_fonts();

  if (s_surface.style.palette) {
    window_set_background_color(
        s_wf_window,
        s_surface.style.palette->background);
  }
  watchface_refresh_strata(WATCHFACE_UPDATE_ALL_STRATA);
}

void watchface_refresh(WatchfaceUpdateMask updates) {
  if (!s_wf_window ||
      !s_wf_settings ||
      updates == WATCHFACE_UPDATE_NONE) {
    return;
  }

  watchface_refresh_strata(updates);
}

static void watchface_refresh_strata(WatchfaceUpdateMask updates) {
  // Don't assume that some strata were created even though that's the contract
  // By not assuming this detail, if the product's must-create strata decision changes,
  // this code won't need to be in sync / will avoid drift.
  if ((updates & WATCHFACE_UPDATE_DATE) &&
      (s_strata_created_mask & DATE_STRATUM_MASK)) {
    date_module_refresh();
  }
  if ((updates & WATCHFACE_UPDATE_TIME) &&
      (s_strata_created_mask & TIME_STRATUM_MASK)) {
    time_module_refresh(s_wf_settings->time_format);
  }
  if ((updates & WATCHFACE_UPDATE_BATTERY) &&
      (s_strata_created_mask & BATTERY_STRATUM_MASK)) {
    battery_module_refresh();
  }
  if ((updates & WATCHFACE_UPDATE_CLIMATE) &&
      (s_strata_created_mask & CLIMATE_STRATUM_MASK)) {
    climate_module_refresh(s_wf_settings->temp_unit);
  }

  #ifdef PBL_HEALTH
  if ((updates & WATCHFACE_UPDATE_HEALTH) &&
      (s_strata_created_mask & BPM_STRATUM_MASK)) {
    bpm_module_refresh();
  }

  if ((updates & WATCHFACE_UPDATE_HEALTH) &&
      (s_strata_created_mask & STEPS_STRATUM_MASK)) {
    steps_module_refresh();
  }
  #endif
}

void watchface_set_temperature(int celsius_tenths) {
  climate_module_set_temperature(celsius_tenths);
}

void watchface_set_weather_condition(int weather_condition) {
  climate_module_set_condition(weather_condition);
}

void watchface_set_is_day(bool is_day) {
  climate_module_set_is_day(is_day);
}

  #if defined(PBL_HEALTH) && (DEBUG_ATAGLANCE == 1)
void watchface_debug_set_bpm(int bpm) {
  bpm_module_debug_set_bpm(bpm);
}

void watchface_debug_set_steps(int steps) {
  steps_module_debug_set_steps(steps);
}

void watchface_debug_clear_health(void) {
  bpm_module_debug_clear_bpm();
  steps_module_debug_clear_steps();
}
#endif
