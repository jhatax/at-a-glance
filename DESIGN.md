# Life at a Glance — Swiss-Rail B2

Pebble Time 2 (emery) · 200×228

## Layout

```
THU · 28 MAY

10:42
────────────────────────────
♥ 62                 [icon] 4218

72°F                          78%
```

| Zone | Content |
|------|---------|
| Top | Short date `THU · 28 MAY` (left rail) |
| Hero | Time, largest type |
| Rule | Hairline under time |
| Complications row | `♥` + BPM left; steps icon + **full** step count right |
| Bottom-left | Temperature (`72°F` or `22°C`) |
| Bottom-right | Battery percent only (no label, no bar) |

## Typography & color

- Left rail: 1px line at x=14, content from x=22
- Primary: off-white for time, BPM number, steps count
- Muted: date, temp, battery %
- Accent: heart icon only (coral)

## Configurable settings

Phone config page (Clay) → AppMessage → watch persists in `persist` or reapplies on each message.

| Setting | Options | Default |
|---------|---------|---------|
| Temperature unit | °F / °C | °F |
| Time format | 12h / 24h | 24h |
| Heart rate sampling | 10 / 15 / 30 / 60 / 120 min | 10 min |

Implementation: `package.json` → `capabilities: ["configurable"]`, message keys for prefs, `src/pkjs/index.js` for Clay UI.

## Data sources

- Time/date: `time()` / `localtime`
- Battery: `battery_state_service`
- Steps / heart rate: `health_service` (on-watch)
- Temperature: Open-Meteo via `src/pkjs/index.js` → AppMessage
- Settings: Clay config → AppMessage → `persist`
