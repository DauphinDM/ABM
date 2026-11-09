# Architecture and compatibility

```text
BeamNG GE Lua bridge (game user folder)
  ├─ reads game state and vehicle-Lua samples at up to 20 Hz
  └─ connected UDP → 127.0.0.1:44444
                       │
                 ABM receiver
                  ├─ validation, heartbeat/session/trip state
                  ├─ bounded local command gateway → bridge
                  ├─ user-only state.json → Omarchy widget
                  └─ local socket stream → Qt Quick dashboard
```

## Source layout

| Location | Responsibility |
| --- | --- |
| `src/main.cpp` | CLI, application style/palette, activation, standalone receiver lifetime |
| `src/telemetry.*` | UDP sanitization, state, local IPC, action gateway, trips |
| `src/backend.*` | QML model, requests/replies, preferences, demo mode |
| `src/integration.*` | Process detection, path discovery, bridge installation, launching |
| `src/previews.*` | Bounded local image registration, loose/archive image reads |
| `src/Main.qml` and components | Basic-style dashboard, dialogs, keyboard/UI behavior |
| `src/theme.*`, `src/translator.*` | Omarchy theme/language integration and fallback |
| `game/` | User-folder GE extension/startup script |
| `plugin/AbmWidget.qml`, `AbmModel.js` | Omarchy popup/bar and bounded parsing |
| `bin/` | Build, test, installation, integration and Windows deployment |
| `pkgbuild/` | Arch package, desktop entry, service and icon |

## Application identity

| Item | Current identity |
| --- | --- |
| Display name | ABM — Advanced BeamNG.Drive Manager |
| Binary / package / desktop ID | `abm` |
| Service | `abm.service` |
| Enable command | `abm-enable` |
| Omarchy plugin / popup IPC | `snobbledev.abm` |
| Windows portable executable | `ABM.exe` |
| Preferences on Linux | `~/.config/abm/` |
| Linux runtime snapshot | `$XDG_RUNTIME_DIR/abm/state.json` |

Windows uses Qt per-user config/cache locations rather than Linux paths. Local
command/activation channels use Qt local sockets (named pipes on Windows).

## Intentionally retained legacy identifiers

The game extension stays at `lua/ge/extensions/beamdeck.lua`, with startup script
`scripts/beamdeck/modScript.lua`, and GE entry `extensions.beamdeck`. JSON wire
envelopes still use `source: "beamdeck"` / `"beamdeck-control"`, schema 1.
Windows local pipe names retain a `beamside-` prefix followed by a path hash.
Environment variable overrides retain `BEAMSIDE_*` names. These are compatibility
details, not current user-facing branding. Changing them independently can
silently disconnect already-installed bridges and test/deployment tooling.

Internal test names and saved vehicle-Lua base-curve fields may also retain
legacy names; the application and release artifacts are ABM.

## Commands and trust

The receiver only forwards an explicit action allowlist. Commands carry a current
session ID and request ID; replies route to their requester. Bridge-side bounds
and ownership/movement checks form a second fence. Catalogs/configurations are
chunked and bounded; arbitrary source code is not accepted as an action payload.

Tuning commands generate fixed vehicle Lua from bounded numeric/boolean values.
Curve baselines live on each engine device to prevent compounded scaling. Beam
limits restore from the vehicle's original JBeam data. Reapplication is scoped
to the previously tuned vehicle ID; gravity is global. The acknowledgement is
not a vehicle-VM completion callback or measured readback.

## Files and persistence

Telemetry/history are runtime/in-memory state. Unit/favorite/tuning requests and
integration paths are stored in per-user INI files. The bar reads the compact
runtime snapshot, while the app receives streamed state. Slow clients/output
queues are bounded. The app does not collect chat logs or network packets.

The enable helper preserves legacy user files and backs up changed integration
files. Bridge installation refuses unrelated files rather than overwriting them.
