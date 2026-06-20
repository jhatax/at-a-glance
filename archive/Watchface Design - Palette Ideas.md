# Palette Archive And Revision Options

This document archives the pre-revision palette, records the selected
Option 1 palette now implemented in code, and preserves Option 2 as a possible
future evaluation direction. It is a design handoff, not a request to ship
multiple options.

## Archived Palette Before Revision

This was the palette before the Shinkansen revision:

```c
static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .background_layer_background = GColorBlack,
  .background_layer_line = GColorWhite,
  .primary_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorRichBrilliantLavender, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .background_layer_background = GColorWhite,
  .background_layer_line = GColorBlack,
  .primary_text = GColorBlack,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorImperialPurple, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorBlack),
};
```

Archived battery dynamic color rule before the naming cleanup and Option 1
normal-state quieting:

```c
static GColor get_battery_color_from_state(void) {
  if (!s_surface || !s_surface->style.palette) {
    return GColorWhite;
  }

  int percent = s_battery_state.charge_percent;

  if (s_battery_state.is_charging) {
    return PBL_IF_COLOR_ELSE(
        GColorJaegerGreen,
        gcolor_legible_over(s_surface->style.palette->background));
  }

  if (percent > 50) {
    return PBL_IF_COLOR_ELSE(
        GColorCobaltBlue,
        gcolor_legible_over(s_surface->style.palette->background));
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(
        GColorRajah,
        gcolor_legible_over(s_surface->style.palette->background));
  }
  return PBL_IF_COLOR_ELSE(
      GColorRed,
      gcolor_legible_over(s_surface->style.palette->background));
}
```

Archived BPM dynamic color rule before Option 1 normal-state quieting:

```c
static GColor calculate_bpm_color(int bpm) {
  if (!s_surface || !s_surface->style.palette) {
    return GColorWhite;
  }

  const ColorPalette* palette = s_surface->style.palette;
  if (bpm <= 0) {
    return palette->unavailable_text;
  }
  if (bpm > 120) {
    return PBL_IF_COLOR_ELSE(
        GColorRed,
        gcolor_legible_over(palette->background));
  }
  if (bpm >= 100) {
    return PBL_IF_COLOR_ELSE(
        GColorMagenta,
        gcolor_legible_over(palette->background));
  }
  return PBL_IF_COLOR_ELSE(
      GColorJaegerGreen,
      gcolor_legible_over(palette->background));
}
```

## Implemented Palette: Option 1, Shinkansen

Current palette in `src/modules/layout_stylist.c`:

```c
static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .background_layer_background = GColorBlack,
  .background_layer_line = GColorWhite,
  .primary_text = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .background_layer_background = GColorWhite,
  .background_layer_line = GColorOxfordBlue,
  .primary_text = GColorCobaltBlue,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .steps_icon = GColorCobaltBlue,
};
```

Current battery dynamic color rule in `src/modules/battery.c`:

```c
static GColor calculate_battery_color(void) {
  if (!s_surface || !s_surface->style.palette) {
    return GColorWhite;
  }

  const ColorPalette* palette = s_surface->style.palette;
  int percent = s_battery_state.charge_percent;

  if (s_battery_state.is_charging) {
    return PBL_IF_COLOR_ELSE(
        GColorJaegerGreen,
        gcolor_legible_over(palette->background));
  }

  if (percent > 50) {
    return palette->primary_text;
  }
  if (percent > 20) {
    return PBL_IF_COLOR_ELSE(
        GColorRajah,
        gcolor_legible_over(palette->background));
  }
  return PBL_IF_COLOR_ELSE(
      GColorRed,
      gcolor_legible_over(palette->background));
}
```

Current BPM dynamic color rule in `src/modules/bpm.c`:

```c
static GColor calculate_bpm_color(int bpm) {
  if (!s_surface || !s_surface->style.palette) {
    return GColorWhite;
  }

  const ColorPalette* palette = s_surface->style.palette;
  if (bpm <= 0) {
    return palette->unavailable_text;
  }
  if (bpm > 120) {
    return PBL_IF_COLOR_ELSE(
        GColorRed,
        gcolor_legible_over(palette->background));
  }
  if (bpm >= 100) {
    return PBL_IF_COLOR_ELSE(
        GColorMagenta,
        gcolor_legible_over(palette->background));
  }
  return palette->primary_text;
}
```

## Font Context

The implemented palette assumes date is no longer visually subordinate to
time. Current `src/c/ataglance.h` uses:

```c
#define DESIGN_FONT_DATE_FULL FONT_KEY_GOTHIC_24_BOLD
```

## Original Option 1 Design Note

Design direction:

- vivid, transit-inspired color system
- Celeste primary data
- Electric Blue date
- Sunset Orange time remains dominant
- normal battery and normal BPM are quieted into primary text
- date test uses Gothic 24 Bold

Palette:

```c
static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .background_layer_background = GColorBlack,
  .background_layer_line = GColorWhite,
  .primary_text = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite),
  .date = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .steps_icon = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite),
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .background_layer_background = GColorWhite,
  .background_layer_line = GColorOxfordBlue,
  .primary_text = GColorCobaltBlue,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
  .date = PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack),
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .steps_icon = GColorCobaltBlue,
};
```

Battery color adjustment:

```c
if (s_battery_state.is_charging) {
  return PBL_IF_COLOR_ELSE(
      GColorJaegerGreen,
      gcolor_legible_over(palette->background));
}
if (percent > 50) {
  return palette->primary_text;
}
if (percent > 20) {
  return PBL_IF_COLOR_ELSE(
      GColorRajah,
      gcolor_legible_over(palette->background));
}
return PBL_IF_COLOR_ELSE(
    GColorRed,
    gcolor_legible_over(palette->background));
```

BPM color adjustment:

```c
if (bpm <= 0) {
  return palette->unavailable_text;
}
if (bpm > 120) {
  return PBL_IF_COLOR_ELSE(
      GColorRed,
      gcolor_legible_over(palette->background));
}
if (bpm >= 100) {
  return PBL_IF_COLOR_ELSE(
      GColorMagenta,
      gcolor_legible_over(palette->background));
}
return palette->primary_text;
```

## Preserved Option 2: Aero-Tactical Palette

Design direction:

- restrained monochrome operational baseline
- Sunset Orange time is the main accent
- normal battery and normal BPM suppress color alarms
- unavailable states use Dark Gray
- date test uses Gothic 24

Palette:

```c
static const ColorPalette c_dark_palette = {
  .background = GColorBlack,
  .background_layer_background = GColorBlack,
  .background_layer_line = GColorWhite,
  .primary_text = GColorWhite,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite),
  .date = GColorWhite,
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorWhite),
  .steps_icon = GColorWhite,
};

static const ColorPalette c_light_palette = {
  .background = GColorWhite,
  .background_layer_background = GColorWhite,
  .background_layer_line = GColorBlack,
  .primary_text = GColorBlack,
  .unavailable_text = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack),
  .date = GColorBlack,
  .time = PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorBlack),
  .steps_icon = GColorBlack,
};
```

Battery color adjustment:

```c
if (s_battery_state.is_charging) {
  return PBL_IF_COLOR_ELSE(
      GColorJaegerGreen,
      gcolor_legible_over(palette->background));
}
if (percent > 50) {
  return palette->primary_text;
}
if (percent > 20) {
  return PBL_IF_COLOR_ELSE(
      GColorRajah,
      gcolor_legible_over(palette->background));
}
return PBL_IF_COLOR_ELSE(
    GColorRed,
    gcolor_legible_over(palette->background));
```

BPM color adjustment:

```c
if (bpm <= 0) {
  return palette->unavailable_text;
}
if (bpm > 120) {
  return PBL_IF_COLOR_ELSE(
      GColorRed,
      gcolor_legible_over(palette->background));
}
if (bpm >= 100) {
  return PBL_IF_COLOR_ELSE(
      GColorMagenta,
      gcolor_legible_over(palette->background));
}
return palette->primary_text;
```

## Evaluation Protocol

Do not ship both options.

Option 1 is currently implemented. If Option 2 is evaluated later, use this
sequence:

1. Commit or stage current source changes before changing palette code.
2. Apply Option 2 only.
3. Build.
4. Install and screenshot Emery in dark mode.
5. Toggle light mode and screenshot Emery in light mode.
6. Optionally install/screenshot a compact rectangular target.
7. Compare against the implemented Shinkansen palette.
8. Select one palette and commit only the selected implementation.

Do not mix palette experiments with layout architect extraction.
