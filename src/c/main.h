#pragma once

#include <pebble.h>
#include "ataglance.h"
#include "../modules/display.h"
#include "../modules/settings.h"

typedef struct {
  Window* window;
} WatchfaceLayerState;

typedef struct {
  WatchfaceSettings settings;
  const VisualPalette* palette;
} WatchfaceRuntimeState;
