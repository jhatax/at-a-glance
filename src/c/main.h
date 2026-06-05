#pragma once

#include <pebble.h>
#include "ataglance.h"
#include "../modules/battery.h"
#include "../modules/date.h"
#include "../modules/display.h"
#if defined(PBL_HEALTH)
#include "../modules/health.h"
#endif
#include "../modules/layout.h"
#include "../modules/settings.h"
#include "../modules/time_display.h"
#include "../modules/weather.h"

typedef struct {
  GFont secondary_value;
  GFont battery_value;
} WatchfaceFontState;

typedef struct {
  Window* window;
} WatchfaceLayerState;

typedef struct {
  WatchfaceSettings settings;
  WatchfaceLayout layout;
  const VisualPalette* palette;
} WatchfaceRuntimeState;
