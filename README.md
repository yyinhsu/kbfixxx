# kbfixxx

[繁體中文](README.zh-TW.md)

> *For those still rocking a pre-2017 MacBook — don't give up on your trusty companion just because of a chattery keyboard. This tool is for you.*

A macOS menu bar app that fixes butterfly keyboard bounce/chatter — the annoying "double key press" issue on MacBook keyboards.

Built as an enhanced alternative to [Unshaky](https://github.com/aahung/Unshaky) with **per-key configurable debounce delay**, **multi-bounce suppression**, **auto-detection of problematic keys**, and a simple JSON config file.

## Features

- **Per-key debounce delay** — Set different delay thresholds (ms) for each key
- **Multi-bounce suppression** — Handle keys that bounce 2–3+ times, not just once
- **Auto-detection** — Automatically identifies keys with rapid repeats and suggests delays
- **Real-time event log** — See every keypress, suppressed or passed, with timestamps
- **Statistics panel** — Per-key suppression counts and bounce rates
- **JSON config** — Human-editable config at `~/.config/kbfixxx/config.json`, hot-reloaded on save
- **Menu bar app** — Lives in the menu bar, no Dock icon

## Installation

### Download (recommended)

1. Go to the [Releases](../../releases/latest) page
2. Download `kbfixxx.app.zip`
3. Unzip and move `kbfixxx.app` to `/Applications`
4. **First launch**: right-click the app → **Open** → click **Open** again (macOS blocks unsigned apps by default). Alternatively, run: `xattr -cr /Applications/kbfixxx.app`
   > ⚠️ Only do this if you trust this project. The app is open-source — you can review the code or [build from source](#build-from-source) yourself.
5. Grant Accessibility permission when prompted

### Build from source

```sh
# Install Xcode Command Line Tools (if not already installed)
xcode-select --install

# Clone the repo
git clone https://github.com/user/kbfixxx.git
cd kbfixxx

# Build the .app bundle
make

# (Optional) Install default config and launch
make run
```

Then move `kbfixxx.app` to `/Applications` and grant Accessibility permission when prompted.

## Requirements

- macOS 10.13 (High Sierra) or later
- Xcode Command Line Tools (`xcode-select --install`)
- **Accessibility permission** (System Settings → Privacy & Security → Accessibility)

## Build & Run

```sh
# Build the app bundle
make

# Build and launch
make run

# Run unit tests
make test

# Clean build artifacts
make clean
```

The app will appear in your menu bar with a keyboard icon. On first launch, it will prompt for Accessibility permission — this is required to intercept keyboard events.

## Configuration

Config file: `~/.config/kbfixxx/config.json`

```json
{
    "global": {
        "ignore_external_keyboard": false,
        "ignore_internal_keyboard": false
    },
    "keys": {
        "b":     { "delay_ms": 60, "max_bounce_count": 1, "enabled": true },
        "space": { "delay_ms": 80, "max_bounce_count": 2, "enabled": true },
        "t":     { "delay_ms": 40, "max_bounce_count": 1, "enabled": true }
    }
}
```

### Per-key settings

| Field | Description | Range |
|-------|-------------|-------|
| `delay_ms` | Debounce window in milliseconds. A second keypress within this window after a keyup is considered a bounce. | 0–2000 |
| `max_bounce_count` | How many consecutive bounces to suppress. Set to 2+ if a key bounces multiple times. | 1–10 |
| `enabled` | Whether debouncing is active for this key. | true/false |

Keys can be specified by name (`b`, `space`, `return`, `tab`, etc.) or by numeric key code (`11`, `49`).

### Picking the right delay

Start with **40 ms** and increase if bounces still slip through. If you type fast (e.g., "apple", "coffee"), a long delay may suppress legitimate double-letter presses. Typical range: **40–80 ms**.

### Global settings

| Field | Description |
|-------|-------------|
| `ignore_external_keyboard` | Skip debouncing for external keyboards |
| `ignore_internal_keyboard` | Skip debouncing for the built-in keyboard |

## How it works

```
Physical key press
    │
    ▼
CGEventTap intercepts keyDown/keyUp
    │
    ▼
Is this key configured for debounce?
    │ No → pass through
    ▼ Yes
Was previous event keyUp within delay_ms?
    │ No → pass through (reset bounce counter)
    ▼ Yes
Increment bounce_counter
    │
    ▼
bounce_counter ≤ max_bounce_count?
    │ No → pass through (genuine fast typing)
    ▼ Yes
SUPPRESS event (return NULL)
```

The app uses `CGEventTapCreate` at the session level to intercept keyboard events before they reach applications. When a bounce is detected (keyDown following keyUp within the configured delay), the event is suppressed and its matching keyUp is also dismissed.

## Menu bar

Click the keyboard icon in the menu bar to access:

- **Enable/Disable** — Toggle debouncing on/off
- **Configure…** — Open the per-key settings window (with "Detect Key" button)
- **Statistics** — View per-key suppression counts and auto-detected bouncing keys
- **Event Log** — Real-time log of all keyboard events
- **Reload Config** — Manually reload the JSON config
- **Open Config File** — Open `config.json` in your default editor

## Architecture

```
src/
├── core/           # Pure C — no Cocoa dependency
│   ├── debouncer   # CGEventTap filter with multi-bounce logic
│   ├── config      # JSON config load/save via cJSON
│   ├── keymap      # 146 macOS virtual key codes ↔ names
│   ├── detector    # Auto-detection of bouncing keys
│   └── stats       # Per-key event/suppression counters
├── app/            # Objective-C — Cocoa UI
│   ├── AppDelegate # Menu bar, event tap lifecycle, FSEvents watcher
│   ├── Preference  # Per-key config table with Detect Key
│   ├── Stats       # Statistics + auto-detection suggestions
│   └── Log         # Real-time scrolling event log
└── vendor/
    └── cJSON       # JSON parser (MIT license)
```

## License

MIT
