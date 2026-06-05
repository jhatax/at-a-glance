#pragma once

#include <pebble.h>

typedef enum {
  ATAGLANCE_USE_AND_PERSIST_SETTINGS = 1,
  ATAGLANCE_MAX_STR_LEN = 16,

  // Reference display used for all layout decisions and icon styling.
  ATAGLANCE_DESIGN_FACE_HEIGHT = 228,
  ATAGLANCE_DESIGN_FACE_WIDTH = 200,

  // Product layout decisions.
  ATAGLANCE_DESIGN_CONTENT_MARGIN = 2,
  ATAGLANCE_DESIGN_ICON_TEXT_GAP = 2,
  ATAGLANCE_DESIGN_ROW_GAP = 8,
  ATAGLANCE_DESIGN_COLUMN_GAP = 8,
  ATAGLANCE_DESIGN_ICON_SIZE = 28
} ATAGLANCE;
