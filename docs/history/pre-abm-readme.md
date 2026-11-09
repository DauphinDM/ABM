# Historical pre-ABM README

This archived document describes earlier development builds. Names, paths,
screenshots and feature claims below are historical, not current instructions.
Use the [current ABM documentation](../../README.md).

## Earlier development notes

A local BeamNG.drive / BeamMP cockpit, trip computer and garage for Omarchy.

**Left-click the bar widget for compact quick stats. Right-click it for the
separate side app.** The app also appears as **ABL** in Super + Space.
Reopening activates the existing window, rather than spawning duplicate windows
or using Hyprland's obsolete `focuswindow` dispatcher.

On this machine a Beamdeck-only rule in `~/.config/hypr/beamdeck.lua`, loaded
from the user `hyprland.lua`, keeps it floating at 500 × 960 rather than being
stretched by default workspace placement. You can still resize or snap the app
yourself. Other apps are unaffected.

## Features

- **Car:** 50–300% torque-curve multiplier, world-gravity slider and planet
  presets, reversible beam deformation protection, repair, fire/extinguish,
  and tire deflation. Use **Apply tuning** after adjusting sliders/protection.
  Preset buttons apply immediately. Requires bridge 0.4.0 and an active vehicle.
  Protection raises beam limits; it is not complete engine/tire invulnerability.
  Values shown are the last requested settings, not a dyno measurement.
- **Visual audit:** `docs/ui-audit.md` records the page/menu capture procedure.

- **Drive:** smoothed speed, RPM/redline arc, gear, fuel percentage/liters,
  coolant, oil, boost (bar), pedals, steering, lateral/longitudinal G,
  lights/signals, ABS/ESC activity and fuel/temperature warnings.
- **Session:** time, estimated distance, top/average speed, acceleration,
  estimated best 0–100 km/h, average/minimum FPS, CPU/GPU frame times,
   speed/FPS history charts and a confirmed trip reset.
- **BeamMP:** connected/joining/single-player detection, cleaned server/player
  titles (no `^l^e` formatting codes), searchable player list, counts, remote
  player pings and vehicle counts. Loading BeamMP alone does not imply a
  multiplayer connection. Missing API values show **—**; some versions do not
  expose your own ping.
- **Garage:** searchable catalog read from the actual game, persisted favorites,
  select an owned active vehicle, or confirm spawning/replacing a model using
  its default configuration. Custom part/configuration editing is not included.
  The list is virtualized rather than constructing every catalog row at once.
- Live Omarchy accent, compact square Basic controls, translucent surfaces,
  persisted units/favorites, and keyboard shortcuts. English is included.

## 60 Hz UI versus game telemetry

The gauge/pedal UI uses Qt Quick's frame clock with a **60 updates/sec target**.
The footer reports its measured update rate. Rendering is limited by your
display and system load; this is not a claim that a slow display or game runs
at 60 FPS. Game telemetry is sampled at **up to 20 Hz**, streamed directly over
a local Unix socket, and interpolated for display. The bar reads a compact
runtime snapshot at 4 Hz. No 60-Hz disk polling or repeated shell commands are
used to animate the UI. Rendering stops while the window is minimized or its
display is paused. Unchanged player/vehicle lists are not rebuilt at 20 Hz.

## Build, install and enable

```sh
bin/build
bin/test
bin/install                 # Build/install the Arch package; normal sudo prompt
bin/enable --json           # Install user files, start receiver, enable widget
```

Default game user folder: `~/.local/share/BeamNG/BeamNG.drive/current`.
Override with `bin/enable --user-folder /path/to/user/folder`.
Existing changed integration files and `shell.json` are backed up before edits.
Nothing in `/usr/share/omarchy/`, BeamMP's managed archives, or the game install
is modified.

### Load / upgrade the game extension

The installer does **not** restart the game. Open its console (usually the
grave/backtick key), select **GE Lua**, and run:

```lua
extensions.unload("beamdeck"); extensions.load("beamdeck")
```

Or restart BeamNG when convenient. The startup script loads the extension on
later launches. The old 0.1 extension can still supply basic 5-Hz stats, but the
garage and new telemetry fields remain unavailable until it is reloaded.

The user-folder files are `lua/ge/extensions/beamdeck.lua` and
`scripts/beamdeck/modScript.lua`. They live outside BeamMP's managed mods folder
because BeamMP deactivates non-server mods. Its whitelist and networking are
not modified. Respect each server's rules for external tools.

## Safe garage behavior

Monitoring and catalog browsing are read-only. Selecting, spawning or replacing
a vehicle requires explicit confirmation in the app and a speed no higher than
1 m/s, checked again by the game extension. Remote-owned vehicles cannot be
selected/replaced in multiplayer. Vehicle requests use `core_vehicles`' public
functions, retaining BeamMP's normal overrides. An acknowledgement means the
request was submitted, not that a server accepted it. Replacement discards the
current vehicle, which is called out in the confirmation. No garage action is
performed automatically or merely to test connectivity.

## Commands / shortcuts

```sh
beamdeck open
beamdeck status --json       # Exit 0 when connected; 1 on disconnect/error
beamdeck preferences --json
beamdeck preferences --units mph --json
beamdeck preferences --favorite etk800 --json  # Toggle a favorite
beamdeck skill install --json # Optional: install the bundled agent guide
beamdeck open --demo         # Labelled samples; never sent to the real receiver
systemctl --user status beamdeck.service
journalctl --user -u beamdeck.service -n 30 --no-pager
```

| Key | Action |
| --- | --- |
| 1–4 | Drive, Session, BeamMP, Garage |
| Space | Pause/resume this display, not the game or receiver |
| U | Switch km/h / mph |
| ? | Keyboard help |
| Escape | Dismiss help/dialog |
| Q | Close dashboard; bar and receiver remain running |

Text searches do not consume the single-letter dashboard shortcuts.
The bar's `shell.json` entry supports `"monitoring": false` to stop displaying
stats there. Use `omarchy bar move snobbledev.beamdeck` to change placement.

## Privacy, limits and validation

The game sends bounded JSON to **127.0.0.1:44444** only. The receiver binds only
IPv4 loopback, validates fields, and writes a private snapshot at
`$XDG_RUNTIME_DIR/beamdeck/state.json`. Streaming/control and dashboard activation
use user-only Unix sockets beside it. Only fixed catalog/spawn/replace/switch
operations and receiver-local trip reset are implemented; there is **no remote
Lua execution**. Game requests are accepted only from the receiver's connected
loopback port, require the current session id and are checked against loaded
catalog entries/game objects. This is a local convenience, not a sandbox against
malicious code already running as your user. Like most local telemetry, another
local process can spoof readings on the UDP port.

No cloud requests, credentials, chat logging or packet interception. Telemetry
stays in runtime storage and a bounded in-memory chart; only preferences persist
in `~/.config/beamdeck/preferences.ini`. Server/player/catalog text is always
plain text, never interpreted as rich text or interpolated into shell commands.
Packets are capped at 32 KiB, players/active vehicles at 128, catalog entries at
2048, and strings/requests/clients/output queues are bounded.

Telemetry expires after three seconds without a heartbeat. Session counters
reset on receiver restart, map change or a stale reconnect. Distance and timing
are speed-sample estimates, not game odometers or official timing. Pausing does
not accumulate distance. Respawns, teleports and unusual physics can invalidate
a 0–100 run. The RPM arc falls back to 8000 if redline is unavailable. Some
engine/fuel readings are not meaningful for electric or modded vehicles.

Built against the locally installed BeamNG 0.40.5 / BeamMP APIs. Tests cover
real UDP/Unix sockets on temporary paths, stale/disconnect handling, command
validation/acknowledgement, mocked garage actions and ownership/movement guards,
caret-code cleanup, markup-shaped input, preferences, real offscreen QML,
shortcuts, frame updates and live translation. Real multiplayer actions must
still be tested by the user on an appropriate server; automated tests never
spawn or replace anything in the live game.

## Disable / remove

```sh
omarchy plugin disable snobbledev.beamdeck
systemctl --user disable --now beamdeck.service
```

In GE Lua: `extensions.unload("beamdeck")`. Remove only the two Beamdeck
user-folder files above to stop future loading. Remove the Arch package normally.
The user service may have a Beamdeck-owned runtime-permission drop-in at
`~/.config/systemd/user/beamdeck.service.d/runtime.conf`. Do not delete BeamNG's
user folder or BeamMP files. Source stays at `~/Work/beamdeck`.
