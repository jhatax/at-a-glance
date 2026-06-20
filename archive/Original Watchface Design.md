## Develop / test / debug

| Task | Command |
|------|---------|
| **Build** | `pebble build` |
| **Emulator** (e.g. Pebble Time) | `pebble install --emulator basalt` |
| **Physical watch** (via phone IP) | `pebble install --phone YOUR_PHONE_IP` |
| **Logs** | `pebble logs --emulator basalt` or `pebble logs --phone IP` |
| **Screenshot** | `pebble screenshot --emulator basalt` |
| **Debug build** | `pebble build --debug` |

---

## Design Prompt

Help me visually design a couple of elegant ways in which I can display the time, battery percentage, heart-beat, step-count, day-date, and current temperature. no code changes yet, let's design together.

## What we’re designing for

| Element | Role | Glance rule |
|--------|------|-------------|
| **Time** | Hero | Largest type; readable in &lt;1 s |
| **Date** | Context | Small, always on; never competes with time |
| **Temp** | Ambient | One number or icon + number; updates slowly |
| **Heart** | Vitals | Icon + BPM; accent color only here (optional) |
| **Steps** | Vitals | Abbreviated (`4.2k`) unless &lt;10k |
| **Battery** | System | % or thin bar; only loud when &lt;20% |

**Shared rules for all concepts**

- **2–3 colors:** base (time), muted (date/secondary), accent (heart — or battery when low).
- **No labels** where icons work: ♥, steps glyph, °, battery pips.
- **Motion later:** subtle pulse on heart icon when BPM updates; nothing flashy on Pebble.

---

## Concept A — Editorial balance (centered, calm)

```
┌────────────────────────────┐
│ THU · 28 MAY          72°  │  ← date left, temp right
│                            │
│         10 : 42            │  ← time hero; colon lighter
│                            │
│  ♥ 62    4.2k steps    78% │  ← one health row + battery
└────────────────────────────┘
```

**Feel:** Magazine cover — symmetrical, calm, “finished.”

**Pros:** Balanced; time is unmistakably primary; everything else in one scan line.
**Cons:** Centered time can feel generic; crowded bottom row if you add labels later.

**Best if:** You want a classic, dress-watch look and rarely add complications.

---

## Concept B — Left rail (Swiss / typographic)

```
┌────────────────────────────┐
│ 10:42                      │  ← left-aligned, heavy
│ ─────────────────────────  │
│ Thursday · 28 May          │
│ ♥ 62 bpm                   │
│ 4,218 steps                │
│ 72°F                  78%█ │
└────────────────────────────┘
```

**Feel:** Instrument panel — rational, modern, very readable.

**Pros:** Strong hierarchy; easy to extend (e.g. weather line); left edge aligns with Pebble’s physical button side.
**Cons:** Less “jewelry”; bottom can get tall if you add more lines.

**Best if:** You care about clarity and might add weather text or calendar hints later.

---

## Concept C — Orbit ring (focused, slightly playful)

```
┌────────────────────────────┐
│        THU 28 MAY          │
│      ┌──────────┐          │
│      │  10:42   │          │  ← time in soft frame
│      └──────────┘          │
│    72°  ♥62  4.2k    (78)  │  ← arc of three + battery ring
└────────────────────────────┘
```

**Feel:** Smartwatch complication — time is “the product,” metrics orbit it.

**Pros:** Distinctive; battery as corner ring saves bottom space.
**Cons:** Hardest to implement well on Pebble; arc layout needs careful pixel alignment.

**Best if:** You want something recognizable and are okay with more custom drawing later.

---

## Cross-cutting choices (pick together)

**1. Time format**
- **24h `10:42`** — clean, European, matches your current code.
- **12h `10:42` + small `PM`** — only if you prefer it; costs a line or corner.

**2. Date format** (pick one)
- `THU · 28 MAY` (compact, A/C)
- `Thursday · 28 May` (warmer, B)
- `28 · MAY` (minimal, no weekday)

**3. Temperature**
- `72°` (no unit clutter; assume your locale)
- `22°C` / `72°F` if you travel or want config

**4. Steps**
- `4.2k` above 10k; full number below
- Optional: thin progress ring toward a daily goal (adds visual weight — use only in one concept)

**5. Battery**
- **% only** (A, C corner) — minimal
- **Thin bar** (B) — faster to read level
- **Hide above 50%** in ambient/low-power modes later

**6. Heart rate**
- Show **BPM only when fresh** (last 1–2 min); gray `—` or hide when stale
- Accent color **only** on ♥ (keeps face calm)

---

## Suggested palette

| Role | Idea |
|------|------|
| Background | Near-black `#000000` or very dark charcoal |
| Primary text | Off-white (not pure white — softer on AMOLED) |
| Muted | ~60% gray for date, temp, steps |
| Accent | Single warm tone for heart (e.g. coral) **or** cool blue if you prefer clinical |
| Warning | Amber/red for battery &lt;20% only |
