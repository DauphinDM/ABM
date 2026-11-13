# Architecture and compatibility

## Development: release updates

`AppUpdates` handles release checks against a fixed repository. Downloads are atomic and checked with SHA-256. The QML `UpdatesPane` holds the opt-in controls. Updates are not part of the bridge or phone command allowlist, and they never run installers or interrupt game control. PC and Android share the implementation. Native iOS `UpdateChecker` only provides opt-in release notifications. Android's non-exported `UpdateProvider` rehashes a private APK and checks its app ID, current signer set, and version code before giving the OS installer a single read-only URI. Publishing and platform installation limits are in [updates](updates.md).

```text
BeamNG GE Lua bridge (game user folder)
  ├─ reads game state and vehicle-Lua samples at up to 20 Hz
  └─ connected UDP → 127.0.0.1:44444
                       │
                 ABM receiver
                  ├─ validation, heartbeat/session/trip state
                  ├─ capped local command gateway → bridge
                  ├─ user-only state.json → Omarchy widget
                  └─ local socket stream → Qt Quick dashboard
```

## Source layout

| Location | Responsibility |
| --- | --- |
| `src/main.cpp` | CLI, application style and palette, activation, standalone receiver lifetime |
| `src/telemetry.*` | UDP sanitization, state, local IPC, action gateway, trips |
| `src/backend.*` | QML model, requests and replies, preferences, demo mode |
| `src/integration.*` | Process detection, path discovery, bridge installation, launching |
| `src/previews.*` | Size-limited local image registration, loose and archive image reads |
| `src/playerprofiles.*` | Explicit public-profile lookup, browser and clipboard actions |
| `src/presetstore.*` | Validated ABM-only preset files, atomic writes, and capped history |
| `src/mobilehost.*`, `src/ConnectPane.qml` | Opt-in private-LAN TLS pairing, approval, and capped remote request gateway |
| `src/companionfirewall.*` | Explicitly confirmed, administrator-approved private-subnet firewall permission. Never started by pairing or remote commands. |
| `src/Main.qml` and components | Basic-style dashboard, dialogs, keyboard and UI behavior |
| `src/theme.*`, `src/translator.*` | Omarchy theme and language integration, with fallback |
| `game/` | User-folder GE bridge, startup script, and vehicle tuning extension |
| `plugin/AbmWidget.qml`, `AbmModel.js` | Omarchy popup and bar, with capped parsing |
| `bin/` | Build, test, installation, integration, and Windows deployment |
| `pkgbuild/` | Arch package, desktop entry, service, and icon |

## Application identity

| Item | Current identity |
| --- | --- |
| Display name | ABM: Advanced BeamNG.Drive Manager |
| Binary / package / desktop ID | `abm` |
| Service | `abm.service` |
| Enable command | `abm-enable` |
| Omarchy plugin / popup IPC | `snobbledev.abm` |
| Windows portable executable | `ABM.exe` |
| Preferences on Linux | `~/.config/abm/` |
| Linux runtime snapshot | `$XDG_RUNTIME_DIR/abm/state.json` |

Windows uses Qt's per-user config and cache locations instead of Linux paths. Local command and activation channels use Qt local sockets, which are named pipes on Windows.

## Legacy identifiers we keep on purpose

The game extension stays at `lua/ge/extensions/beamdeck.lua`, with startup script `scripts/beamdeck/modScript.lua` and GE entry `extensions.beamdeck`. JSON wire envelopes still use `source: "beamdeck"` and `"beamdeck-control"`, schema 1. Windows local pipe names keep a `beamside-` prefix followed by a path hash. Environment variable overrides keep `BEAMSIDE_*` names. These are compatibility details, not current user-facing branding. Changing them on their own can silently disconnect installed bridges and break test or deployment tooling.

Some internal test names and saved vehicle-Lua base-curve fields also keep legacy names. The application and release artifacts are ABM.

## Commands and trust

Development builds add an **off-by-default** phone connection inside the dashboard process, not the receiver. `MobileHost` listens on a selected private IPv4 address using TLS 1.2 or later. A short-lived invitation pins the locally generated certificate and carries a random secret. The phone still needs explicit approval on the PC. Approval can optionally issue a revocable random token. The PC stores only its hash, with at most 16 remembered devices. Explicit resume still requires the pinned PC certificate and an enabled listener. Stop disconnects. Forget revokes saved tokens.

`UsbTunnel` uses optional iproxy or adb helpers and size-limited loopback byte relays. It does not terminate TLS or open a LAN port. Android uses no-rebind and cleans up only its own mapping on the same device. Game UDP, the command socket, and arbitrary Lua are never exposed.

Snapshots use a field allowlist, a 100 ms cadence, backpressure, and library deltas. Opaque preview IDs resolve only against the current library. Local paths are replaced with JPEG thumbnails capped at 320×200 and 64 KiB. Commands use a fixed allowlist, size-limited frames, monotonic sequences, and a frozen target. Mutations need ownership, permission, and a known stopped speed. Spawning into a loaded empty map doesn't need an existing car. Backend, receiver, and bridge validation still apply. A receipt is not physics verification.

The separate [ABM-Mobile repository](https://github.com/DauphinDM/ABM-Mobile) contains the Android and native SwiftUI clients. See [mobile companion](mobile-companion.md) for setup, limits, and security details. These features ship from 0.13.0. The older 0.12.0 binaries don't contain them.

The receiver only forwards an explicit action allowlist. Commands carry a current session ID and request ID, and replies go back to the requester. Bridge-side limits plus ownership and movement checks form a second fence. Catalogs and configurations are chunked and size-limited. Arbitrary source code is never accepted as an action payload.

Tuning commands generate fixed vehicle Lua from validated numeric and boolean values. Curve baselines live on each engine device so scaling doesn't compound. Beam limits restore from the vehicle's original JBeam data. Reapplication is scoped to the previously tuned vehicle ID. Gravity is global. Bridge 0.5 dispatches to `lua/vehicle/extensions/abmTuning.lua` and routes completion callbacks that match the vehicle ID and request ID. Game-side timeouts are 10 seconds, and duplicate suppression is capped. Reapplication is silent. Older fire, repair, and tire actions still report queued or submitted work. None of these acknowledgements is measured readback.

Engine power has no artificial multiplier ceiling. The vehicle preflights derived torque for finite results. Handling values have explicit ranges. Gear arrays are limited by a 24 KB serialized budget, not a fixed gear count. Manual-gearbox indexed tables grow together. An extension reset hook restores their original sizes before the game's own powertrain reset runs.

## Session actions

Session roster conversion uses BeamMP's documented extension modules, with legacy global aliases only as a fallback. Ownership checks use local game vehicle IDs. Session player IDs and explicit guest flags are separate. A missing ID is not authentication evidence. Profile lookup context includes the bridge session, server endpoint and name, row key, and raw username. Roster changes and clipboard writes cancel pending copies. When snapshots are unavailable, profile state clears before the UI is notified. See [Session action review](session-actions.md) for API references and isolated verification limits.

## Files and persistence

Preview registration builds up to eight approved local candidates. It keeps the game's reported mounted origin before same-path loose or stock fallbacks. Each candidate must decode successfully. Bytes existing is not enough. Paths, sizes, and modification times feed the opaque source-revision IDs when catalogs are decorated again. No game placeholder path is guessed.

The provider keeps the existing 8 MiB encoded-image limit, 8192-pixel axis limit, 16-million-pixel decode limit, and 100,000-entry archive scan limit. Thumbnails are capped at 960×640 and use a 64 MiB-cost LRU cache, with a 1 KiB minimum per entry, including failures. Cache bookkeeping and transient decoder allocations sit on top of that. Misses are serialized to avoid duplicate decoding. No thumbnails are written to disk. QML's fallback selection keeps its source binding when a Garage card is reused for another configuration.

Development 0.7 adds `ServerPolicy` and a bundled, frozen official-server roster. Since 0.7.1, approval checks exact raw names only, which allows host and port rotations. The receiver associates the current endpoint and name with a short-lived HTTPS directory response for official classification. It denies unverified multiplayer mutations and sends session-bound leases valid for at most three seconds. The game bridge independently checks the same frozen name roster, the current connection identity, and expiry. It gates direct commands and reapply, and starts owned-vehicle restoration or reset when permission changes. Cleanup errors keep commands locked. Read [server-policy.md](server-policy.md) for expiration, network privacy, reset side effects, and non-anticheat limits.

Telemetry and history are runtime, in-memory state. Unit, favorite, and tuning requests plus integration paths are stored in per-user INI files. The bar reads the compact runtime snapshot. The app receives streamed state. Slow clients and output queues are capped. ABM doesn't collect chat logs or network packets.

The enable helper preserves legacy user files and backs up integration files before changing them. Bridge installation refuses unrelated files instead of overwriting them.
