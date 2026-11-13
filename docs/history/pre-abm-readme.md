# Historical pre-ABM README

This archived document describes earlier development builds. The names, paths,
screenshots and feature claims below are historical, not current instructions.
Use the [current ABM documentation](../../README.md).

## Earlier development notes

A local BeamNG.drive / BeamMP cockpit, trip computer and garage for Omarchy.

Left-click the bar widget for compact quick stats. Right-click it for the
separate side app. The app also appears as ABL in Super + Space. Reopening it
activates the window that's already open instead of spawning a duplicate, and
it doesn't use Hyprland's obsolete `focuswindow` dispatcher.

On this machine, a Beamdeck-only rule in `~/.config/hypr/beamdeck.lua`, loaded
from the user `hyprland.lua`, keeps the app floating at 500 × 960. Without the
rule, default workspace placement stretches it. You can still resize or snap
the app yourself, and the rule doesn't affect other apps.

## Features

- **Car:** 50-300% torque-curve multiplier, world-gravity slider and planet
  presets, reversible beam deformation protection, repair, fire/extinguish
  and tire deflation. Use **Apply tuning** after adjusting sliders or
  protection. Preset buttons apply right away. You need bridge 0.4.0 and an
  active vehicle. Protection raises beam limits; it doesn't make the engine or
  tires fully invulnerable. The values shown are the last settings you
  requested, not a dyno measurement.
- **Visual audit:** `docs/ui-audit.md` records how to capture pages and menus.

- **Drive:** smoothed speed, RPM/redline arc, gear, fuel percentage and liters,
  coolant, oil, boost (bar), pedals, steering, lateral and longitudinal G,
  lights and signals, ABS/ESC activity, and fuel/temperature warnings.
- **Session:** time, estimated distance, top and average speed, acceleration,
  estimated best 0-100 km/h, average and minimum FPS, CPU/GPU frame times,
  speed and FPS history charts, and a trip reset that asks for confirmation.
- **BeamMP:** connected, joining and single-player detection; cleaned server and
  player titles (no `^l^e` formatting codes); searchable player list; counts;
  remote player pings and vehicle counts. Loading BeamMP by itself doesn't mean
  you're connected to multiplayer. Missing API values show a dash. Some
  versions don't expose your own ping.
- **Garage:** a searchable catalog read from the actual game, favorites that
  persist, selection of an owned active vehicle, or a confirmed spawn/replace
  of a model using its default configuration. Custom part and configuration
  editing isn't included. The list is virtualized instead of building every
  catalog row at once.
- Live Omarchy accent, compact square Basic controls, translucent surfaces,
  persisted units and favorites, and keyboard shortcuts. English is included.

## 60 Hz UI versus game telemetry

The gauge and pedal UI uses Qt Quick's frame clock with a target of 60 updates
per second. The footer reports the rate it actually measures. Rendering is
limited by your display and system load. That isn't a claim that a slow display
or the game runs at 60 FPS. Game telemetry is sampled at up to 20 Hz, streamed
straight over a local Unix socket and interpolated for display. The bar reads a
compact runtime snapshot at 4 Hz. Nothing polls the disk at 60 Hz, and the UI
doesn't run repeated shell commands to animate. Rendering stops while the
window is minimized or its display is paused. Player and vehicle lists that
haven't changed aren't rebuilt at 20 Hz.

## Build, install and enable

```sh
bin/build
bin/test
bin/install                 # Build/install the Arch package; normal sudo prompt
bin/enable --json           # Install user files, start receiver, enable widget
```

The default game user folder is
`~/.local/share/BeamNG/BeamNG.drive/current`. Override it with
`bin/enable --user-folder /path/to/user/folder`. If integration files or
`shell.json` have changed, they're backed up before the edits. Nothing in
`/usr/share/omarchy/`, BeamMP's managed archives or the game install is
modified.

### Load / upgrade the game extension

The installer won't restart the game. Open its console (usually the
grave/backtick key), select **GE Lua**, and run:

```lua
extensions.unload("beamdeck"); extensions.load("beamdeck")
```

Or restart BeamNG when it's convenient. The startup script loads the extension
on later launches. The old 0.1 extension can still supply basic 5-Hz stats, but
the garage and new telemetry fields stay unavailable until you reload it.

The user-folder files are `lua/ge/extensions/beamdeck.lua` and
`scripts/beamdeck/modScript.lua`. They sit outside BeamMP's managed mods folder
because BeamMP deactivates non-server mods. We didn't modify its whitelist or
networking. Follow each server's rules for external tools.

## Safe garage behavior

Monitoring and catalog browsing are read-only. Selecting, spawning or replacing
a vehicle requires you to confirm in the app, and the vehicle must be moving no
faster than 1 m/s. The game extension checks the speed again. You can't select
or replace a remote-owned vehicle in multiplayer. Vehicle requests go through
`core_vehicles`' public functions, so BeamMP's normal overrides still apply. An
acknowledgement means the request was submitted, not that a server accepted it.
Replacement discards the current vehicle, and the confirmation says so. No
garage action happens automatically, and none runs just to test connectivity.

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
| 1-4 | Drive, Session, BeamMP, Garage |
| Space | Pause/resume this display, not the game or receiver |
| U | Switch km/h / mph |
| ? | Keyboard help |
| Escape | Dismiss help/dialog |
| Q | Close dashboard; bar and receiver keep running |

Text searches don't trigger the single-letter dashboard shortcuts. The bar's
`shell.json` entry supports `"monitoring": false` to stop showing stats there.
Use `omarchy bar move snobbledev.beamdeck` to change its placement.

## Privacy, limits and validation

The game sends JSON to **127.0.0.1:44444** only. The receiver binds IPv4
loopback only, validates fields, and writes a private snapshot at
`$XDG_RUNTIME_DIR/beamdeck/state.json`. Streaming and control, plus dashboard
activation, use user-only Unix sockets next to it. Only fixed catalog, spawn,
replace, switch and receiver-local trip-reset operations exist. There is **no
remote Lua execution**. Game requests are accepted only from the receiver's
connected loopback port, must carry the current session ID, and are checked
against loaded catalog entries and game objects. This is a local convenience,
not a sandbox against malicious code already running as your user. As with most
local telemetry, another local process can spoof readings on the UDP port.

There are no cloud requests, credentials, chat logging or packet interception.
Telemetry stays in runtime storage and a size-limited in-memory chart. Only
preferences persist, in `~/.config/beamdeck/preferences.ini`. Server, player and
catalog text is always plain text, never interpreted as rich text or inserted
into shell commands. Packets are capped at 32 KiB, players and active vehicles
at 128, catalog entries at 2048, and strings, requests, clients and output
queues all have fixed limits.

Telemetry expires after three seconds without a heartbeat. Session counters
reset on receiver restart, map change or a stale reconnect. Distance and timing
are estimates from speed samples, not game odometers or official timing.
Pausing doesn't accumulate distance. Respawns, teleports and unusual physics
can invalidate a 0-100 run. The RPM arc falls back to 8000 if redline isn't
available. Some engine and fuel readings don't mean much for electric or modded
vehicles.

Built against the locally installed BeamNG 0.40.5 / BeamMP APIs. Tests cover
real UDP and Unix sockets on temporary paths, stale and disconnect handling,
command validation and acknowledgement, mocked garage actions with ownership
and movement guards, caret-code cleanup, markup-shaped input, preferences, real
offscreen QML, shortcuts, frame updates and live translation. You still need to
test real multiplayer actions on an appropriate server; the automated tests
never spawn or replace anything in the live game.

## Disable / remove

```sh
omarchy plugin disable snobbledev.beamdeck
systemctl --user disable --now beamdeck.service
```

In GE Lua: `extensions.unload("beamdeck")`. To stop future loading, remove only
the two Beamdeck user-folder files listed above. Remove the Arch package the
normal way. The user service may have a Beamdeck-owned runtime-permission
drop-in at `~/.config/systemd/user/beamdeck.service.d/runtime.conf`. Don't
delete BeamNG's user folder or BeamMP files. Source stays at `~/Work/beamdeck`.
