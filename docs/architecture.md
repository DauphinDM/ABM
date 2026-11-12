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
| `src/playerprofiles.*` | Explicit public-profile lookup, browser and clipboard actions |
| `src/presetstore.*` | Validated ABM-only preset files, atomic writes and bounded history |
| `src/mobilehost.*`, `src/ConnectPane.qml` | Opt-in private-LAN TLS pairing, approval and bounded remote request gateway |
| `src/Main.qml` and components | Basic-style dashboard, dialogs, keyboard/UI behavior |
| `src/theme.*`, `src/translator.*` | Omarchy theme/language integration and fallback |
| `game/` | User-folder GE bridge, startup script and vehicle tuning extension |
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

Development builds add an **off-by-default** phone connection in the dashboard
process, not the receiver. `MobileHost` listens on a selected private IPv4 address
using TLS 1.2 or later. A short-lived invitation pins the locally generated
certificate and carries a random secret; a phone still needs explicit PC approval.
Approval is connection-only and stopping access revokes it. This does not expose
the game's UDP port, command socket, filesystem or arbitrary Lua.

Snapshots use a field allowlist and omit local preview paths. Commands use a fixed
allowlist, bounded frames, monotonic sequence numbers and a frozen game/vehicle
target. Mutations require current ownership, permission and a known stopped speed,
then pass through the existing backend/receiver/bridge validation. Receipts are
not physics verification. The separate [ABM-Mobile repository](https://github.com/DauphinDM/ABM-Mobile)
contains the Android client; see [mobile companion](mobile-companion.md) for setup,
limitations and security details. This is not part of the published 0.12.0 binaries.

The receiver only forwards an explicit action allowlist. Commands carry a current
session ID and request ID; replies route to their requester. Bridge-side bounds
and ownership/movement checks form a second fence. Catalogs/configurations are
chunked and bounded; arbitrary source code is not accepted as an action payload.

Tuning commands generate fixed vehicle Lua from validated numeric/boolean values.
Curve baselines live on each engine device to prevent compounded scaling. Beam
limits restore from the vehicle's original JBeam data. Reapplication is scoped
to the previously tuned vehicle ID; gravity is global. Bridge 0.5 dispatches to
`lua/vehicle/extensions/abmTuning.lua` and routes vehicle-ID/request-ID-matched
completion callbacks, with a 10-second game-side timeout and bounded duplicate
suppression. Reapplication is silent. Old fire/repair/tire actions still report
queued/submitted work. None of these acknowledgements is measured readback.

Engine power has no artificial multiplier ceiling; the vehicle preflights
derived torque for finite results. Handling values have explicit ranges.
Gear arrays are limited by a 24 KB serialized budget, not a fixed gear count.
Manual-gearbox indexed tables grow together; an extension reset hook restores
their original dimensions before the game's own powertrain reset runs.

## Session actions

Session roster conversion uses BeamMP's documented extension modules, with legacy
global aliases only as a fallback. Ownership checks use local game vehicle IDs.
Session player IDs and explicit guest flags are separate; absent IDs are not
authentication evidence. Profile lookup context includes bridge session, server
endpoint/name, row key and raw username. Roster changes and clipboard writes
cancel pending copies; unavailable snapshots clear profile state before UI
notification. See [Session action review](session-actions.md) for API references
and isolated verification limits.

## Files and persistence

Preview registration builds up to eight approved local candidates, preserving
the game's reported mounted origin before same-path loose/stock fallbacks. Each
candidate must decode successfully; bytes existing is not sufficient. Paths,
sizes and modification times contribute to opaque source-revision IDs when
catalogs are decorated again. No game placeholder path is guessed.

The provider retains the existing 8 MiB encoded-image limit, 8192-pixel axis limit,
16-million-pixel decode limit and 100,000-entry archive scan limit. Thumbnails are
bounded to 960×640 and use a 64 MiB-cost LRU cache (minimum 1 KiB per entry, including
failures); cache bookkeeping and transient decoder allocations are additional.
Misses are serialized to avoid concurrent duplicate decoding. No thumbnails are
written to disk. QML's fallback selection keeps its source binding intact when
a Garage card is reused for another configuration.

Development 0.7 adds `ServerPolicy` and a bundled, frozen official-server roster.
Since 0.7.1, approval checks exact raw names only, permitting host/port rotations.
The receiver associates the current endpoint/name with a short-lived HTTPS
directory response for official classification, denies unverified multiplayer mutations, and sends session-
bound leases valid for at most three seconds. The game bridge independently
checks the same frozen name roster, current connection identity and expiry, gates direct commands and
reapply, and initiates owned-vehicle restoration/reset on permission changes.
Cleanup errors keep commands locked. Read [server-policy.md](server-policy.md)
for expiration, network privacy, reset side effects and non-anticheat limits.

Telemetry/history are runtime/in-memory state. Unit/favorite/tuning requests and
integration paths are stored in per-user INI files. The bar reads the compact
runtime snapshot, while the app receives streamed state. Slow clients/output
queues are bounded. The app does not collect chat logs or network packets.

The enable helper preserves legacy user files and backs up changed integration
files. Bridge installation refuses unrelated files rather than overwriting them.
