#include "watchface.h"

#include "battery.h"
#include "climate.h"
#include "date.h"
#include "modules/helper.h"
#include "substratum_renderer.h"
#include "time.h"
#include "watchface_layout.h"

#ifdef PBL_HEALTH
#include "bpm.h"
#include "steps.h"
#endif

/*
 * File invariants:
 *
 * - live WatchfaceSurface owner
 *   This file owns the active prepared surface, its runtime-visible style, and
 *   the lifecycle of watchface strata built from that surface.
 *
 * - watchface orchestration owner
 *   This file owns create/destroy order, repaint, and targeted refresh
 *   dispatch into feature modules.
 *
 * - consume resolved runtime inputs only
 *   This file may use already-resolved settings and refresh masks, but it must
 *   not interpret raw Pebble callbacks or AppMessage tuples.
 *
 * - no Pebble OS, transport, or persistence ownership
 *   Service subscription, AppMessage parsing, and settings load/save policy
 *   belong to ataglance.c and its Pebble-message adapter helpers.
 *
 * - no layout-calculation ownership
 *   Layout preparation, geometry calculation, and styling policy belong to the
 *   layout facade, architect, and stylist.
 *
 * - module refresh stays narrow
 *   Feature modules receive only the palette and runtime scalars they need, not
 *   the whole watchface surface or raw transport data.
 */

#define ARE_CUSTOM_FONTS_LOADED() ((s_surface.style.fontbook.custom_fonts_loaded_count) > 0)

static WatchfaceSurface s_surface = {0};

// Font lifecycle management
static bool s_fonts_initialized = false;
static Window* s_wf_window = NULL;
static const WatchfaceSettings* s_wf_settings = NULL;
static bool s_watchface_initialized = false;
static const WatchfaceUpdateMask WATCHFACE_UPDATE_ALL_STRATA =
    WATCHFACE_UPDATE_TIME | WATCHFACE_UPDATE_DATE | WATCHFACE_UPDATE_BATTERY |
    WATCHFACE_UPDATE_CLIMATE | WATCHFACE_UPDATE_LOCATION | WATCHFACE_UPDATE_BLUETOOTH |
    PBL_IF_HEALTH_ELSE(WATCHFACE_UPDATE_HEALTH, WATCHFACE_UPDATE_NONE);

typedef enum {
  NO_STRATA_MASK = 0,
  DATE_STRATUM_MASK = 1 << 0,
  TIME_STRATUM_MASK = 1 << 1,
  BATTERY_STRATUM_MASK = 1 << 2,
  CLIMATE_STRATUM_MASK = 1 << 3,
  BTICON_STRATUM_MASK = 1 << 4,
#ifdef PBL_HEALTH
  BPM_STRATUM_MASK = 1 << 5,
  STEPS_STRATUM_MASK = 1 << 6,
#endif
  MUST_HAVE_STRATA_MASK =
      DATE_STRATUM_MASK | TIME_STRATUM_MASK | BATTERY_STRATUM_MASK | CLIMATE_STRATUM_MASK,
} WatchfaceStratumMask;

static WatchfaceStratumMask s_strata_created_mask = NO_STRATA_MASK;
static GBitmap* s_nobt_icon = NULL;
static Layer* s_nobt_icon_layer = NULL;

// Function declarations
static void watchface_load_and_apply_palette(void);
static void watchface_load_and_apply_fonts(void);

static void watchface_load_and_apply_palette() {
  // Update the palette
  layout_watchface_update_palette(&(s_surface.style), s_wf_settings->display_mode);

  // set the background color according to palette
  if (s_surface.style.palette) {
    window_set_background_color(s_wf_window, s_surface.style.palette->background);
  }
}

static void watchface_load_and_apply_fonts() {
  if (!s_fonts_initialized) {
    s_fonts_initialized = layout_watchface_initialize_fonts(&s_surface.style.fontbook);

    if (s_fonts_initialized && !ARE_CUSTOM_FONTS_LOADED()) {
      layout_watchface_load_custom_fonts(&s_surface.style.fontbook);
    }
  }
}

static void nobt_icon_update_proc(
    Layer* layer,
    GContext* ctx) {
  if (!layer || !ctx || !s_nobt_icon) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  static GColor l_icon_color = GColorBlack;
  GColor background = s_surface.style.palette->background;

  graphics_context_set_fill_color(ctx, background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  GColor maybe_color = s_surface.style.palette->outofrange_text;
  if (!HELPER_COLOR_EQUAL(l_icon_color, maybe_color)) {
    if (helper_replace_color_in_bitmap(s_nobt_icon, l_icon_color, maybe_color)) {
      l_icon_color = maybe_color;
    }
  }
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_nobt_icon, bounds);

  substratum_renderer_mark_info_outofrange(ctx, &bounds, l_icon_color, background);
}

static void initialize_bt_icon(
    Layer* root) {
  s_nobt_icon = gbitmap_create_with_resource(RESOURCE_ID_BT);
  if (s_nobt_icon) {
    s_nobt_icon_layer =
        substratum_renderer_create_icon_layer(root, &s_surface.bt_icon.icon, nobt_icon_update_proc);
    if (!s_nobt_icon_layer) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create No Bluetooth icon layer");
      gbitmap_destroy(s_nobt_icon);
      s_nobt_icon = NULL;
      s_strata_created_mask &= ~BTICON_STRATUM_MASK;
    } else {
      layer_set_hidden(s_nobt_icon_layer, true);
      s_strata_created_mask |= BTICON_STRATUM_MASK;
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to create No Bluetooth icon bitmap");
  }
}

// Lifecycle Ownership responsibility
bool watchface_create(
    Window* window,
    const WatchfaceSettings* settings) {
  if (s_watchface_initialized) {
    return true;
  }

  // At this point, we have confirmed that s_watchface_initialized is false.
  // This in-function boolean is going to track whether all modules are initialized.
  // Wait until the end to set s_watchface_initialized to the final value of this bool.
  bool success = true;
  if (success && !(window && settings)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Cannot create watchface without watchface inputs");
    success = false;
  }

  Layer* root = NULL;
  if (success) {
    s_wf_window = window;
    s_wf_settings = settings;

    root = window_get_root_layer(window);
  }

  if (success && !root) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Main window root layer is NULL");
    success = false;
  }

  if (success) {
    s_strata_created_mask = NO_STRATA_MASK;

    GRect bounds = layer_get_bounds(root);
    if (!layout_watchface_prepare(bounds.size.w, bounds.size.h, &s_surface)) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface layout initialization failed");
      success = false;
    }
  }
  bool created = false;
  if (success) {
    // Load & Apply Style
    watchface_load_and_apply_palette();

    // Load & Apply Fonts (one-time activity for every launch)
    watchface_load_and_apply_fonts();

    created = date_module_create(root,
        &s_surface.date.text,
        s_surface.style.fontbook.chosen_fonts[s_surface.date.text.font_role]);
    s_strata_created_mask |= (created) ? DATE_STRATUM_MASK : 0;

    created = time_module_create(root,
        &s_surface.time.text,
        s_surface.style.fontbook.chosen_fonts[s_surface.time.text.font_role]);
    s_strata_created_mask |= created ? TIME_STRATUM_MASK : 0;

    created = battery_module_create(root, &s_surface.battery);
    s_strata_created_mask |= (created) ? BATTERY_STRATUM_MASK : 0;

    created = climate_module_create(root,
        &s_surface.climate.text,
        &s_surface.location.text,
        &s_surface.climate.icon,
        s_surface.style.fontbook.chosen_fonts[s_surface.climate.text.font_role],
        s_surface.style.fontbook.chosen_fonts[s_surface.location.text.font_role]);
    s_strata_created_mask |= created ? CLIMATE_STRATUM_MASK : 0;

    if ((s_strata_created_mask & MUST_HAVE_STRATA_MASK) != MUST_HAVE_STRATA_MASK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Watchface must-initialize controls failed");
      success = false;
    }
  }

  s_watchface_initialized = success;

#ifdef PBL_HEALTH
  if (s_watchface_initialized) {
    initialize_bt_icon(root);
    created = bpm_module_create(root,
        &s_surface.bpm.text,
        &s_surface.bpm.icon,
        s_surface.style.fontbook.chosen_fonts[s_surface.bpm.text.font_role]);
    s_strata_created_mask |= (created) ? BPM_STRATUM_MASK : 0;

    created = steps_module_create(root,
        &s_surface.steps.text,
        &s_surface.steps.icon,
        &s_surface.steps.progress,
        s_surface.style.fontbook.chosen_fonts[s_surface.steps.text.font_role]);
    s_strata_created_mask |= (created) ? STEPS_STRATUM_MASK : 0;
  }
#endif

  if (s_watchface_initialized) {
    watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA);
  }
  return s_watchface_initialized;
}

void watchface_destroy() {
  // Failed create can leave partial module state before initialization completes.
  date_module_destroy();
  s_strata_created_mask &= ~DATE_STRATUM_MASK;

  time_module_destroy();
  s_strata_created_mask &= ~TIME_STRATUM_MASK;

  battery_module_destroy();
  s_strata_created_mask &= ~BATTERY_STRATUM_MASK;

  climate_module_destroy();
  s_strata_created_mask &= ~CLIMATE_STRATUM_MASK;

  if (s_nobt_icon) {
    gbitmap_destroy(s_nobt_icon);
    s_nobt_icon = NULL;
  }

  if (s_nobt_icon_layer) {
    layer_destroy(s_nobt_icon_layer);
    s_nobt_icon_layer = NULL;
  }
  s_strata_created_mask &= ~BTICON_STRATUM_MASK;

#ifdef PBL_HEALTH
  if (s_strata_created_mask & BPM_STRATUM_MASK) {
    bpm_module_destroy();
    s_strata_created_mask &= ~BPM_STRATUM_MASK;
  }
  if (s_strata_created_mask & STEPS_STRATUM_MASK) {
    steps_module_destroy();
    s_strata_created_mask &= ~STEPS_STRATUM_MASK;
  }
  // End Health Capability check
#endif

  if (ARE_CUSTOM_FONTS_LOADED()) {
    layout_watchface_unload_custom_fonts(&s_surface.style.fontbook);
  }

  s_strata_created_mask = (uint8_t)NO_STRATA_MASK;
  s_wf_settings = NULL;
  s_wf_window = NULL;
  s_fonts_initialized = false;
  s_watchface_initialized = false;

  // Zero out the watchface surface which will wipe everything else out
  memset(&s_surface, 0, sizeof(s_surface));
}

// Watch face update orchestration responsibility
void watchface_repaint() {
  if (!s_wf_window || !s_wf_settings) {
    return;
  }

  // Re-style
  watchface_load_and_apply_palette();
  watchface_refresh(WATCHFACE_UPDATE_ALL_STRATA);
}

void watchface_refresh(
    WatchfaceUpdateMask updates) {
  if (!s_wf_window || !s_wf_settings || updates == WATCHFACE_UPDATE_NONE) {
    return;
  }

  // Treat s_strata_created_mask as the sole source of truth for stratum creation success.
  // Using any other mask will create a hidden dependency / source of drift
  // Three exceptions to settings propagation handled outside this function:
  // 1. display_mode: watchface_repaint reloads the palette
  // 2. weather_update_minutes: index.js in the handler for web-view closed
  // 3. hr_sample_minutes: ataglance.c does this when settings are subscribed or
  // changed on devices with health capability.
  if ((updates & WATCHFACE_UPDATE_DATE) && (s_strata_created_mask & DATE_STRATUM_MASK)) {
    date_module_refresh(s_surface.style.palette);
  }
  if ((updates & WATCHFACE_UPDATE_TIME) && (s_strata_created_mask & TIME_STRATUM_MASK)) {
    time_module_refresh(s_surface.style.palette, s_wf_settings->time_format);
  }
  if ((updates & WATCHFACE_UPDATE_BATTERY) && (s_strata_created_mask & BATTERY_STRATUM_MASK)) {
    battery_module_refresh(s_surface.style.palette);
  }
  if (s_strata_created_mask & CLIMATE_STRATUM_MASK) {
    if (updates & WATCHFACE_UPDATE_CLIMATE) {
      climate_module_refresh(s_surface.style.palette,
          WATCHFACE_UPDATE_CLIMATE,
          s_wf_settings->temp_unit);
    }

    if (updates & WATCHFACE_UPDATE_LOCATION) {
      climate_module_refresh(s_surface.style.palette,
          WATCHFACE_UPDATE_LOCATION,
          s_wf_settings->temp_unit);
    }
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Checking BT status");
  if ((updates & WATCHFACE_UPDATE_BLUETOOTH) && (s_strata_created_mask & BTICON_STRATUM_MASK)) {
    bool bt_state = connection_service_peek_pebble_app_connection();
    if (!bt_state) {
      layer_set_hidden(s_nobt_icon_layer, false);
      APP_LOG(APP_LOG_LEVEL_INFO, "BT status is disconnected; icon shown");
      vibes_double_pulse();
    } else {
      layer_set_hidden(s_nobt_icon_layer, true);
      APP_LOG(APP_LOG_LEVEL_INFO, "BT status is connected; icon hidden");
    }
  }

#ifdef PBL_HEALTH
  if ((updates & WATCHFACE_UPDATE_HEALTH)) {
    if (s_strata_created_mask & BPM_STRATUM_MASK) {
      bpm_module_refresh(s_surface.style.palette);
    }

    if (s_strata_created_mask & STEPS_STRATUM_MASK) {
      steps_module_refresh(s_surface.style.palette, s_wf_settings->steps_goal);
    }
  }
#endif
}
