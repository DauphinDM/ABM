# Changelog

## Unreleased — opt-in app updates

- Add official-release checks and optional SHA-256 verified downloads on PC/Android, with preview selection, response/transfer bounds and safe cancellation. Default settings do not contact GitHub.
- Add native iOS release notifications; sideloaded IPAs still require local re-signing or an Apple distribution channel. PC installation remains explicit, never an automatic game interruption.
- Add a restricted Android APK-installer handoff and optional stable CI release signing through maintainer secrets; mobile packaging advances to build 4.
- Test release/asset selection, hostile URLs, digest/size failures, redirects, cancellation and opt-in preferences using synthetic network responses.

## Unreleased — mobile usability and USB

- Send bounded on-demand Garage thumbnails and retain unchanged libraries between 10 Hz telemetry updates; display phone-to-PC round-trip latency separately from game ping.
- Permit validated spawning into a loaded empty map without requiring an existing car. Ownership, stopping, policy and target fences remain where relevant.
- Add optional remembered-device tokens with explicit reconnect, PC-side hash-only storage and revocation. Stop disconnects; Forget all devices revokes saved access.
- Add explicit iPhone iproxy and Android adb USB modes using loopback-only relays with the same certificate-pinned TLS. Helpers are optional and existing Android forwards are not replaced. Physical cable operation still needs testing.
- Mobile actions apply directly without redundant Send to PC confirmation; keyboards have explicit dismissal. Security approval and administrator firewall confirmation remain.
- Add TLS/thumbnail/resume/revocation/empty-map and mocked-helper byte-bridge tests. Fix a Windows macro collision in firewall adapter lookup.

## Unreleased — companion firewall setup

- PC Connect now explains stalled secure connections and offers explicit, administrator-approved LAN firewall setup for Linux UFW and Windows Private profiles. Rules use the selected interface's actual subnet and bound TCP address/port, never all networks or router forwards. Other firewalls/masks remain manual; persistent-rule scope and removal are explained before confirmation.
- Added scope/command/PowerShell-encoding and real QML confirmation/cancellation/demo-safety tests; no test modifies the machine's firewall. Existing-window activation now honors the Connect tab too.
## Unreleased — Android companion foundation

- Add a fifth **Connect** screen and opt-in private-IPv4 TLS host for the separate [ABM Mobile](https://github.com/DauphinDM/ABM-Mobile) Android app. This is not included in the 0.12.0 release assets.
- Require a short-lived random pairing link, certificate pinning and explicit PC approval; stop revokes access. No cloud relay, automatic firewall changes, saved approval or public listener.
- Route only fixed remote operations through existing Backend/game validation, with target identity, stopped/owned/policy gates, bounded messages, sequence and rate checks. No shell, arbitrary Lua or direct game UDP exposure.
- Add isolated TLS/approval/target/ownership/movement/policy/revocation tests. Android/device and live-game validation remain pending; mobile is an early subset rather than full desktop parity.

## 0.12.0 — Dashboard and Session actions (preview)

- Publish public Linux and Windows preview downloads with SHA256 checksums and dependency notices. Keep bridge compatibility at 0.11.0; update/install the bundled bridge files for Session fixes.
- Fix the preset backend self-binding detected by Windows Qt 6.8 CI; explicitly bind the application backend and assert it in real-window tests.
- Keep text-field, text-area and editable-combo context menus compatible with Qt 6.8; test right-click copying and keyboard opening. Explicitly constrain responsive content width and wait for settled layout geometry in CI tests.
- Report the release version consistently in CLI help, `--version` and support bundles.
- Isolate CLI skill installation explicitly on Windows, where Qt's native profile lookup ignores a temporary `HOME`; preserve overwrite protection and reject relative override directories. Treat identical reinstalls as a no-write success, avoiding Windows open-file replacement failures.
- Wait briefly for an already-starting window's activation listener instead of rejecting a simultaneous launch; test early and subsequent activation on both Windows and Linux without bypassing the single-window lock.

### Session action fixes

- Resolve BeamMP's documented `extensions` APIs consistently for roster telemetry, ownership and command authorization; retain legacy global fallback. Read session player IDs from the documented `getPlayers()` map keys and stop inferring guest authentication from missing IDs.
- Bind public-profile lookups to bridge/server/row/name identity, cancel on stale or changed rosters, and clear profile state when telemetry is unavailable. Show per-player action results and lookup state; sample players do not open real profiles or trigger network lookups.
- Check clipboard retention, format IPv6 server addresses correctly, explain disabled vehicle selection and report switching as a request rather than confirmed game acceptance.
- Add real-window button/outcome tests during changing telemetry, isolated browser/forum-response tests, and strict extension-only BeamMP ownership/policy mocks. Preserve movement, ownership and server-policy restrictions.

### Dashboard redesign

- Replace the decorative glass shell with a quieter dark dashboard, clearer hierarchy, larger instruments, consistent controls and stronger text contrast.
- Add adaptive tabs/sidebar navigation, a spacious default window, grouped Workshop panels and player cards. Short windows prioritize content over page headings.
- Give the Garage larger photo cards, an adaptive one-to-four-column grid, separate scrollable filters and keyboard-accessible card/favorite actions. Keep preview provenance and all game-action guards.
- Refresh the command palette, dialogs, menus and checkboxes; give confirmations explicit action labels. Add a bounded demo-only `--dev-size WIDTHxHEIGHT` screenshot option and responsive layout/keyboard regression coverage.

### Garage preview reliability

- Try decoding each approved same-image candidate before falling back, including corrupt mounted/archive images and unreadable or oversized loose files.
- Preserve image bindings when Garage cards change configuration; show **Model preview** only for a successfully loaded model fallback.
- Add a bounded memory-only thumbnail/failure cache with source-metadata revision IDs, serialized cache misses and read-time path checks. Existing 8 MiB encoded-image and 16-million-pixel limits remain.
- Add deterministic image/fallback/card-reuse regression tests and a design-to-code review. Native mounted precedence and the game's default placeholder remain unverified; no game files or simulation state were changed.

## 0.11.1 — Ratio editing beyond manual gearboxes

- Apply ratios and add or remove forward gears on manual, automatic, DCT and sequential transmissions while preserving reverse and neutral.
- Re-engage the previously selected gear after resizing, so Apply no longer leaves the car revving in neutral.
- Support CVT as a single fixed-ratio edit; reset restores the stock variator range when persistence is enabled.
- Show the detected transmission type and enable `+ Gear` only where gear-count editing is supported.
- Ratio edits still require a stopped, owned vehicle and remain target-bound and reset-aware.
- Clutch protection now also raises supported friction-clutch clamp capacity 20× so high-power tunes can transfer torque; heat/wear stay frozen, but other drivetrain shock and damage remain possible. Reset restores the original clamp.

## 0.11.0 — Latency, players and presets

- Capture BeamMP's own local/remote ping updates without editing BeamMP files; show provenance and freshness, never treat `-1` as latency.
- Add stable session player rows with guest/session identity, role and vehicle counts, search/sort, public forum profile links, separate session-ID copying, account-ID lookup with cancellation guards, server address copying, convoy notes and honest multiplayer policy text.
- Add bounded ABM-owned presets that store accepted workshop overrides only. Validation, atomic writes, history, tags/notes, import/export, explicit apply preflight, stopped-own-vehicle requirements, partial-result reporting and cancellation are included. Presets are not full native `.pc` clones.
- Add supported torque-curve and clutch readback, gear spread, seeded preview-first tune drafts, preview fallback labels and a redacted support bundle.
- Require game bridge and vehicle bridge 0.11.0 for new controls. Older bridges stay read-only where supported.
- Native BeamNG save/load pairing, real latency comparison and physics effects remain unverified because the simulator/Lua bridge could not be connected during development.

## 0.10.0 — Reset-recipe development

- Edit supported combustion-engine permanent RPM limits with actual idle/ceiling readback. Preserve physical damage thresholds and temporary ESC/launch limiters; do not synthesize torque curves.
- Protect supported friction clutches from additional thermal/wear/deformation damage. Freeze their thermal update without increasing torque capacity or silently repairing an already damaged clutch.
- Keep accepted power, beam protection, strength, RPM, clutch, handling, manual gears and experimental dimensions across ordinary resets of the same loaded vehicle. Restore once after readiness and authorization checks; stop on failure rather than retrying a mutation loop.
- Add a per-vehicle persistence toggle and restoration status/error readback. Full reset and policy cleanup clear pending restoration before resetting physics. Recipes are memory-only, not automatic cross-vehicle presets.
- Show estimated RPM after an upshift beside actual/edited gear ratios. This is a ratio calculation, not a performance prediction.
- Require bridge/vehicle extension 0.10.0 for these additions. Existing exact-name official-server restrictions remain in force.
- Native MCP discovery worked, but the running simulator could not be connected; real-game reset/clutch/geometry effects remain unverified.

## 0.9.0 — Foundation development

- Separate live engine-power commands: bounded latest-value scheduling, no beam/gravity edits, finite-overflow checks.
- Versioned vehicle-instance readback, engine RPM information, actual signed manual-gear ratios and an editable per-gear overview.
- Suggested extra ratios use recent progression rather than a fixed 0.85 factor; suggestions are not performance optima.
- Target/session/generation/expiry checks for new workshop commands, with cancellation on disconnect, policy loss and vehicle-instance changes.
- Bottom-of-Car full reset cancels pending power, restores ABM vehicle adapters and requests a stopped physics reset; world gravity is separate.
- This is roadmap slice A0, not the entire massive update. Editable RPM, clutch protection and persistent reset recipes remain later work.


## 0.8.0 — Private preview

- Add combinable Garage vehicle type, stock/mod/custom content and favorites
  filters; brand, body style, configuration type, drivetrain, transmission,
  source/mod, country, fuel and propulsion facets, plus six sort orders.
- Forward bounded real game metadata and size-aware catalog chunks. Model
  facets cover any of their configurations; unknown metadata is not guessed.
- Consolidate the workshop into four related sections. Read vehicle capability
  flags and disable unsupported, unsafe-at-speed, invalid or policy-locked
  controls; keep bounce stop available on restricted servers.
- Keep bridge protocol 0.7.1 and the existing official-server policy unchanged.
  Updated bridge files and a game restart are needed for capability discovery.

## 0.7.1 — Development snapshot

- Approve frozen regular official server names independently of IP/port, so
  host rotations are supported. Both receiver and game bridge use exact raw
  names; old addresses do not restrict verified non-official replacements.
- Keep event/new official names blocked and unknown classification locked.
  Current-connection leases still require fresh verification after a rotation.
- Require bridge 0.7.1 for multiplayer. Name reuse/impersonation, including an
  event adopting an approved name, cannot be distinguished by this policy.

## 0.7.0 — Development snapshot

- Freeze 108 regular official BeamMP identities (IP, port and exact raw name),
  excluding the four MOZA qualifying servers. New/mismatched official servers
  are denied; verified non-official servers remain unrestricted by this policy.
- Classify through BeamMP's public HTTPS directory, fail closed when unverified,
  and enforce bounded identity-bound leases in both receiver and game bridge.
- Lock gameplay actions, stop reapply, cancel pending commands, restore captured
  values and request owned-car physics resets on permission/identity changes.
  Keep telemetry/browsing and bounce stop available. Cleanup errors remain locked.
- Require bridge 0.7 for multiplayer mutations; bundle the fourth roster file.
  Add a visible lock reason and blocked-server demo capture.
- This is not tamper-proof anti-cheat or permission from a server. Approved
  identities repurposed without changes cannot be detected. Live transitions
  and cleanup remain unverified; see docs/server-policy.md.

## 0.6.2 — Development snapshot

- Fix the confirmed `selectShiftPoints` nil-entry crash beyond factory top gear:
  extend the controller shift-point table alongside gearbox/synchro/sound tables.
  Added gears inherit stock top-gear thresholds. Reject unsupported controller
  closures before mutation, restore stock on reset, roll back inertia failures.
- Block gear edits on pre-0.6.2 bridges, which still contain the crash.
- Allow stopped, owned-car resizing in BeamMP. Geometry is local and is **not
  synchronized to peers**; server rules still apply. No server bypass or remote
  vehicle edits. Real-game driving and peer behavior remain unverified.
- Correct portable CI's Qt 6.8 architecture from `gcc_64` to `linux_gcc_64`;
  the previous job failed installing Qt, before compilation. CI pending.

## 0.6.1 — Development snapshot

- Add a separate Stop bouncing button using the real `vehicle:why("stop")`
  command. No reset, guessed toggle state, or repair is needed.
- Add independent 0.5–2× width, length and height sliders. Experimental
  physical-node/rest-length scaling, preserving captured preload and mass;
  stopped, single-player and unbroken-beam guards. Reset restores stock.
  Controllers/tires/props are not rebuilt; real-game behavior remains unverified.
- Require bridge 0.6.1 for both additions; retain older action compatibility.
- Canonicalize Windows CLI-test temp/skill paths before checking containment,
  accommodating 8.3 aliases without weakening the isolated-HOME requirement.
  The preceding Windows run passed all Qt tests and failed this path assertion;
  the updated Windows job still needs to pass.

## 0.6.0 — Development snapshot

- Add a 0.1–100× combustion-engine strength slider and immediate protection
  toggle. Scale stock torque tolerance and over-torque/over-rev damage budgets
  without changing power or the rev limiter. Restore hooks and stock fields
  on reset; reject unsupported engines before changing any engine.
- Engine protection blocks the verified damage hooks, flooding and over-rev/
  over-torque failures, and freezes thermal updates. It does not repair damage
  or protect the whole drivetrain; real-game validation remains outstanding.
- Add a pinned-tool AppImage/bundled-archive recipe and Ubuntu/Debian/Fedora/Arch
  startup matrix. Experimental until CI passes; not a promise of all Linux.
- Document researched sandbox expansions and their game API/ownership limits.
- Require bridge 0.6 for engine durability; retain older action compatibility.

## 0.5.0 — Development snapshot

- Split the Car workshop into Power & world, Grip & handling, Gearbox, and Fun.
- Keep the power slider and add positive numeric percentages without a fixed cap.
  Reject non-finite input and derived torque overflow before mutation.
- Add static/sliding tire grip, brake strength and steering-response multipliers,
  plus the supported vehicle ESC/traction-mode cycle. Skip damaged tires.
- Add editable forward ratios and an Add Gear button for verified manual
  gearboxes. No fixed gear-count cap; a 24 KB serialized command budget applies.
  Grow synchro/sound tables together and restore stock on vehicle reset.
- Add the real `why("this dealer")` bounce command when the game exposes it.
- Report vehicle-VM completion/errors for new sandbox actions and tuning rather
  than treating a queued script as successful. Legacy fun actions still queue.
- Preserve 0.3/0.4 compatibility and require bridge 0.5 for the new features.
- Real-game effects remain unverified; this does not change release v0.4.0.

## 0.4.0 — 2026-11-09

First private GitHub preview release as **ABM — Advanced BeamNG.Drive Manager**.

### Included

- Native Qt Quick Basic dashboard: Drive, Car, Garage and Session.
- Engine torque multiplier, world-gravity slider/planet presets, reversible
  beam protection, repair, fire/extinguish and tire-deflation actions.
- Real installed model/configuration photos, favorites, custom configurations,
  safe spawn/replace/switch confirmations and ownership checks.
- Local telemetry receiver, trip estimates, speed/FPS history and diagnostics.
- Steam/BeamMP launch controls, user-folder bridge installer and Omarchy widget.
- Standalone receiver lifecycle and Windows Qt/MSVC deployment workflow.
- Extended installation, migration, user, CLI, architecture, security,
  troubleshooting and development documentation.
- ABM binary/package/service/desktop/widget/Windows release branding.

### Validation and boundaries

- 24 Qt tests plus Lua bridge, plugin, CLI and standalone suites before the
  rename; all suites are rerun for the ABM release.
- Screenshot review of all pages, dialogs/menus and the live bar popup.
- Windows runtime and real-game tuning effects are not yet certified.
- Legacy wire/bridge/environment identifiers retained intentionally; see
  `docs/architecture.md`. No complete ECU editor or automatic updater.
