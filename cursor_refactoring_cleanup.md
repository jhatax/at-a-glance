# Refactoring & Cleanup
_Exported on 5/28/2026 at 17:58:30 PDT from Cursor (3.5.38)_

---

**User**

Refactor the selected code so that all fonts are retrieved at the beginning. better yet, create global font variables, mark them as static as you have other global variables, retrieve them once during this function, and use them where appropriate vs. loading them on demand.

---

**Cursor**

Reading `main.c` to see how fonts are loaded and how other globals are declared.


Adding static global `GFont` variables and loading them once at the start of `main_window_load`.


Verifying the project builds:


Refactored `main_window_load` so fonts are loaded once and reused.

**Static globals** (with the other module state):

```21:24:/Users/manomeht/Code/MM-Watchface/src/c/main.c
static GFont s_font_gothic_14;
static GFont s_font_bitham_42_bold;
static GFont s_font_gothic_18_bold;
static GFont s_font_gothic_18;
```

**One-time load** at the top of `main_window_load`, before colors and layers:

```279:282:/Users/manomeht/Code/MM-Watchface/src/c/main.c
  s_font_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_font_bitham_42_bold = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  s_font_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_gothic_18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
```

**Usage:** each `text_layer_set_font` now takes the matching `s_font_*` instead of calling `fonts_get_system_font` again. There are four distinct system fonts (Gothic 14, Bitham 42 Bold, Gothic 18 Bold, Gothic 18), so four globals cover all seven layers.

`pebble build` completes successfully.

---

**User**

do fonts have to be unloaded as well or just the text layers and canvases?

---

**Cursor**

Checking how the Pebble SDK treats system fonts versus custom fonts.


**System fonts** (what you use via `fonts_get_system_font`) do **not** need to be unloaded. The SDK owns them; `fonts_get_system_font` just returns a handle. There is no `fonts_unload_system_font` — only `fonts_unload_custom_font` for fonts you load yourself.

**Custom fonts** (from `fonts_load_custom_font` / a resource) **do** need `fonts_unload_custom_font` when you are done with them. Do that in `main_window_unload` **after** destroying any `TextLayer` (or other layer) that still references that font, since destruction may still touch the font.

Your watchface only uses system fonts, so your unload path is enough:

- `text_layer_destroy` for text layers  
- `layer_destroy` for plain layers (rail, rule, steps icon)

You do not need extra font cleanup for `s_font_gothic_14`, etc. Those globals are just cached handles; clearing them on unload is optional and does not free memory.

**Layers vs “canvases”:** On Pebble you tear down UI by destroying layers (`text_layer_destroy`, `layer_destroy`, `bitmap_layer_destroy`, etc.). There is no separate canvas/font unload for system fonts. If you later add custom fonts or bitmaps, pair `gbitmap_destroy` / `fonts_unload_custom_font` with the same unload handler, still after the layers that use them.

---

**User**

refactor the code so that we are very safe with using strings in this embedded context. Start by defining two constants: MAX_STR_LEN (12) and TOTAL_BUFFERS (the number of strings you will need). Then, define a set of ENUMs that refers to each type of character array with the final value referring to CLEANUP. Create a static text buffer manager function that creates the right number of static character buffers (as a 2D array), returns a clean char* pointer to the buffer based on the index in the ENUM after checking for boundaries correctly. Avoid index overflows, zero stack issues, and zero dynamic memory allocations to keep RAM usage fixed. Finally, set all arrays to NULL during window_unload so that there are no stale artifacts.

---

**Cursor**

Reading `main.c` to locate all string buffers and how they're used.


Implementing a centralized static text-buffer manager and refactoring all update functions to use it.


Refactored string handling around a fixed static buffer pool. Summary:

### Constants and enum
- `MAX_STR_LEN` (12) — each slot holds up to 11 characters plus `'\0'`.
- `TextBufferId` — `BUF_DATE`, `BUF_TIME`, `BUF_BPM`, `BUF_STEPS`, `BUF_BATTERY`, `BUF_TEMP`, then `BUF_CLEANUP` as the count sentinel.
- `TOTAL_BUFFERS` — `((size_t)BUF_CLEANUP)` → 6 slots.

### Storage (BSS, no heap, no per-call stack buffers)
```c
static char s_text_buffers[TOTAL_BUFFERS][MAX_STR_LEN];
```
Fixed RAM: **72 bytes** for all UI strings.

### Manager
- **`text_buffer_get(id)`** — rejects out-of-range IDs with `NULL`; otherwise clears the slot (`buf[0] = '\0'`) and returns that `char*`.
- **`text_buffer_cleanup()`** — `memset` of the whole pool to zero (C char arrays can’t be “NULL”; this clears them so nothing stale remains after unload).

### Call sites
All `update_*` paths use `text_buffer_get` + `MAX_STR_LEN` for `strftime` / `snprintf` / `format_*`, with early return if `get` fails.

### Unload
`text_buffer_cleanup()` runs at the end of `main_window_unload`, after layers are destroyed.

Build succeeds. **Note:** the date format `"%a · %d %b"` can be 13 characters for two-digit days (e.g. `WED · 31 DEC`). With `MAX_STR_LEN == 12`, `strftime` truncates safely but the last character may be dropped on those days. If that shows up in testing, bump only `BUF_DATE` or `MAX_STR_LEN` slightly.

---

**User**

Increase the maximum string length to be 16.

---

**Cursor**

Updated `MAX_STR_LEN` to **16**. Every buffer slot and `strftime`/`snprintf` call already uses that macro, so the text pool is now 6 × 16 = **96 bytes** of fixed RAM. Build still passes.

---

**User**

@MM-Watchface/src/c/main.c:61-63 refactor this to use a global ENUM and then a case..switch for readability and compiler optimization.

---

**Cursor**

Fixing a pre-existing sign-compare warning in `text_buffer_get` so the build passes.


Introduced a module-level `HrSampleMinutes` enum with values matching persisted minutes (15, 30, 60, 120), and validation now uses a `switch`:

```17:24:/Users/manomeht/Code/MM-Watchface/src/c/main.c
typedef enum {
  HR_SAMPLE_MINUTES_15 = 15,
  HR_SAMPLE_MINUTES_30 = 30,
  HR_SAMPLE_MINUTES_60 = 60,
  HR_SAMPLE_MINUTES_120 = 120,
} HrSampleMinutes;

#define HR_SAMPLE_MINUTES_DEFAULT HR_SAMPLE_MINUTES_15
```

```68:79:/Users/manomeht/Code/MM-Watchface/src/c/main.c
static bool hr_sample_minutes_valid(int minutes) {
  switch ((HrSampleMinutes)minutes) {
    case HR_SAMPLE_MINUTES_15:
    case HR_SAMPLE_MINUTES_30:
    case HR_SAMPLE_MINUTES_60:
    case HR_SAMPLE_MINUTES_120:
      return true;
    default:
      return false;
  }
}
```

`HR_SAMPLE_MINUTES_DEFAULT` is now `HR_SAMPLE_MINUTES_15`, so defaults and persisted settings stay compatible with the phone config (still sends 15/30/60/120). Build passes.

---

**User**

@MM-Watchface/src/c/main.c:103-115 refactor this to use a case..switch like you just did for the other function.

---

**Cursor**

Refactored `text_buffer_get` to use a `switch` on `TextBufferId`:

```103:120:/Users/manomeht/Code/MM-Watchface/src/c/main.c
static char *text_buffer_get(TextBufferId id) {
  switch (id) {
    case BUF_DATE:
    case BUF_TIME:
    case BUF_BPM:
    case BUF_STEPS:
    case BUF_BATTERY:
    case BUF_TEMP:
      return s_text_buffers[id];
    case BUF_CLEANUP:
      for (size_t i = 0; i < TOTAL_BUFFERS; ++i) {
        s_text_buffers[i][0] = '\0';
      }
      return NULL;
    default:
      return NULL;
  }
}
```

Valid buffer IDs share one return path; `BUF_CLEANUP` clears every slot; anything else hits `default` and returns `NULL`. Build still passes.

---

**User**

build

---

**Cursor**

Build finished successfully. Output bundle: `build/MM-Watchface.pbw` (nothing needed recompiling this run).

---

**User**

how can I set the phone's location to fallback to OAK or SFO in case location isn't available?

---

**Cursor**

Exploring how the watchface gets location and temperature from the phone.


Location fallback is handled on the **phone** in `src/pkjs/index.js`, not in the C watchface code.

### Where it’s defined

```6:7:/Users/manomeht/Code/MM-Watchface/src/pkjs/index.js
var DEFAULT_LAT = 40.7128;
var DEFAULT_LON = -74.006;
```

Those are NYC. Open-Meteo is called with them when:

- `getCurrentPosition` **errors** (denied, timeout, etc.)
- **`navigator.geolocation`** isn’t available
- (Not when GPS works — then it uses the phone’s real position)

### Set fallback to SFO or OAK

Change `DEFAULT_LAT` / `DEFAULT_LON` to one fixed point (airport-ish):

| Place | Latitude | Longitude |
|--------|----------|-----------|
| **SFO** (airport) | `37.6213` | `-122.3790` |
| **OAK** (airport) | `37.7213` | `-122.2208` |

Example for **SFO**:

```javascript
var DEFAULT_LAT = 37.6213;
var DEFAULT_LON = -122.3790;
```

Example for **OAK**:

```javascript
var DEFAULT_LAT = 37.7213;
var DEFAULT_LON = -122.2208;
```

Rebuild and reinstall so the phone loads the new companion JS:

```bash
pebble build
pebble install --phone YOUR_PHONE_IP   # or your usual install path
```

Update **README.md** line 40 if you change the documented fallback (it still says NYC).

### “OAK or SFO” as a choice

The app only supports **one** default pair today. To pick at runtime you’d add something like:

- A Clay **select** (`WEATHER_FALLBACK`: SFO / OAK), and in the `getCurrentPosition` error callback (and the no-geolocation branch) use the matching lat/lon, or  
- Try geolocation first, then SFO, then OAK (more API calls; usually unnecessary in the Bay Area).

### Phone permissions

Fallback only runs when location fails. On iOS/Android, allow location for the Pebble/Rebble app (or the companion) so you get real local weather when GPS works.

I can patch `index.js` (and README) to SFO or OAK if you say which you want as the single default.

---

**User**

replace mm-watchface with life-at-a-glance-pebble-watchface in whatever artifacts will get uploaded to github.

---

**Cursor**

Searching the repo for naming references to update for GitHub.


Updating source and documentation files that would be committed to GitHub.


---

**User**

build

---

**Cursor**

Build completed successfully.

Output: `build/MM-Watchface.pbw` (the `.pbw` filename still follows the project folder name `MM-Watchface`, not `package.json`). After you rename the repo directory to `life-at-a-glance-pebble-watchface`, rebuild and you should get `build/life-at-a-glance-pebble-watchface.pbw`.

