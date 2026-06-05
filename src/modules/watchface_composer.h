#pragma once

#include <pebble.h>

#include "display.h"
#include "settings.h"

void watchface_composer_init(void);
bool watchface_composer_create(
    Window* window,
    const WatchfaceSettings* settings,
    const VisualPalette* palette);
void watchface_composer_destroy(void);
void watchface_composer_refresh(
    Window* window,
    const WatchfaceSettings* settings,
    const VisualPalette* palette);
