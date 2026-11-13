# Changelog

## 0.14.0: Browser control for older devices

- Added an embedded HTTPS control page on the existing Connect listener. Older phones, tablets, and computers on the private LAN can use telemetry, workshop actions, Garage spawn and replace, session switching, and explicit disconnect without installing ABM Mobile.
- Pairing still needs the same two-minute secret and explicit PC approval. The browser gets an HttpOnly, Secure, SameSite=Strict 12-hour session cookie. Only one controller is allowed at a time, and remembered-browser tokens can be revoked.
- HTTP handling accepts one bounded request per TLS connection and checks Host, Origin, and a custom header. Bodies must use Content-Length. Responses set no-store and strict CSP, COOP, and CORP headers. Browser commands go through the existing exact target, sequence, rate, ownership, stopped, sandbox, and server-policy checks. No arbitrary RPC or thumbnails are exposed.
- Added QtTest coverage for static assets, pairing, pending and approved state, one-controller refusal, target rejection, unpair, saved-browser resume, and malformed or oversized requests.
- Fixed the MSVC build error in browser asset routing. Browser requests now time out after five seconds, and failed state polls clear stale readings and disable game controls until fresh state arrives. Timed-out commands aren't replayed.
- Browser disconnect now clears the PC's connected-device status. A HEAD request no longer consumes the saved-browser credential before the browser can retrieve it, and reconnect timeouts no longer delete saved access. Added client regression tests to Linux and Windows checks.

## 0.13.0: App updates and mobile companions (preview)

Ship PC 0.13.0 with the matching [ABM Mobile 0.2.0](https://github.com/DauphinDM/ABM-Mobile/releases) release. Previously published PC 0.12.0 assets stay available and are not replaced.

- Added official-release checks and optional SHA-256 verified downloads on PC and Android, with preview selection, response and transfer limits, and safe cancellation. Default settings don't contact GitHub.
- Added native iOS release notifications. Sideloaded IPAs still need local re-signing or an Apple distribution channel. PC installation stays explicit and never interrupts your game.
- Added a restricted Android APK installer handoff and optional stable CI release signing through maintainer secrets. Mobile packaging moves to build 5.
- Tested release and asset selection, hostile URLs, digest and size failures, redirects, cancellation, and opt-in preferences using synthetic network responses.
- Fixed a native iOS false disconnect when a long main-thread stall delayed snapshot delivery. Commands still stop when telemetry is stale, and the watchdog now confirms a real outage before clearing the connection.

### Mobile usability and USB

- Send limited on-demand Garage thumbnails and keep unchanged libraries between 10 Hz telemetry updates. Show phone-to-PC round-trip latency separately from game ping.
- Allow validated spawning into a loaded empty map without needing an existing car. Ownership, stopping, policy, and target checks remain where they matter.
- Added optional remembered-device tokens with explicit reconnect, PC-side hash-only storage, and revocation. Stop disconnects. Forget all devices revokes saved access.
- Added explicit iPhone iproxy and Android adb USB modes using loopback-only relays with the same certificate-pinned TLS. Helpers are optional, and existing Android forwards aren't replaced. Physical cable operation still needs testing.
- Mobile actions apply directly without a redundant Send to PC confirmation. Keyboards have explicit dismissal. Security approval and administrator firewall confirmation remain.
- Added TLS, thumbnail, resume, revocation, empty-map, and mocked-helper byte-bridge tests. Fixed a Windows macro collision in firewall adapter lookup.

### Companion firewall setup

- PC Connect now explains stalled secure connections and offers explicit, administrator-approved LAN firewall setup for Linux UFW and Windows Private profiles. Rules use the selected interface's actual subnet and bound TCP address and port. They never cover all networks or create router forwards. Other firewalls and masks remain manual. Persistent-rule scope and removal are explained before confirmation.
- Added scope, command, and PowerShell encoding tests plus real QML confirmation, cancellation, and demo-safety tests. No test modifies the machine's firewall. Existing-window activation now honors the Connect tab too.

### Android companion foundation

- Added a fifth **Connect** screen and an opt-in private-IPv4 TLS host for the separate [ABM Mobile](https://github.com/DauphinDM/ABM-Mobile) Android app. This isn't in the 0.12.0 release assets.
- Requires a short-lived random pairing link, certificate pinning, and explicit PC approval. Stopping revokes access. There's no cloud relay, automatic firewall change, saved approval, or public listener.
- Routes only fixed remote operations through existing Backend and game validation, with target identity, stopped, owned, and policy gates, limited messages, and sequence and rate checks. No shell, arbitrary Lua, or direct game UDP exposure.
- Added isolated TLS, approval, target, ownership, movement, policy, and revocation tests. Android device and live-game validation remain pending. Mobile is an early subset, not full desktop parity.

## 0.12.0: Dashboard and Session actions (preview)

- Published public Linux and Windows preview downloads with SHA256 checksums and dependency notices. Bridge compatibility stays at 0.11.0. Update or install the bundled bridge files for the Session fixes.
- Fixed the preset backend self-binding that Windows Qt 6.8 CI caught. The application backend now sets its own bindings, and real-window tests assert them.
- Kept text-field, text-area, and editable-combo context menus compatible with Qt 6.8. Right-click copying and keyboard opening are tested. Responsive content width is limited, and CI tests wait for settled layout geometry.
- The release version now appears consistently in CLI help, `--version`, and support bundles.
- Isolated CLI skill installation on Windows, where Qt's native profile lookup ignores a temporary `HOME`. Overwrite protection stays, relative override directories are rejected, and an identical reinstall is treated as a no-write success so Windows open-file replacement doesn't fail.
- When a window is already starting, ABM now waits briefly for its activation listener instead of rejecting a simultaneous launch. Early and later activation are tested on Windows and Linux without bypassing the single-window lock.

### Session action fixes

- Resolve BeamMP's documented `extensions` APIs consistently for roster telemetry, ownership, and command authorization, keeping the legacy global fallback. Read session player IDs from the documented `getPlayers()` map keys and stop inferring guest authentication from missing IDs.
- Bind public-profile lookups to bridge, server, row, and name identity. Cancel on stale or changed rosters, and clear profile state when telemetry is unavailable. Show per-player action results and lookup state. Sample players don't open real profiles or trigger network lookups.
- Check clipboard retention, format IPv6 server addresses correctly, explain disabled vehicle selection, and report switching as a request rather than confirmed game acceptance.
- Added real-window button and outcome tests during changing telemetry, isolated browser and forum-response tests, and strict extension-only BeamMP ownership and policy mocks. Movement, ownership, and server-policy restrictions remain.

### Dashboard redesign

- Replaced the decorative glass shell with a quieter dark dashboard, clearer hierarchy, larger instruments, consistent controls, and stronger text contrast.
- Added adaptive tabs and sidebar navigation, a spacious default window, grouped Workshop panels, and player cards. Short windows prioritize content over page headings.
- Gave the Garage larger photo cards, an adaptive one-to-four-column grid, separate scrollable filters, and keyboard-accessible card and favorite actions. Preview provenance and all game-action guards remain.
- Refreshed the command palette, dialogs, menus, and checkboxes. Confirmations now name the action. Added a limited demo-only `--dev-size WIDTHxHEIGHT` screenshot option plus responsive layout and keyboard regression coverage.

### Garage preview reliability

- Try to decode each approved same-image candidate before falling back, including corrupt mounted or archive images and unreadable or oversized loose files.
- Preserve image bindings when Garage cards change configuration. Show **Model preview** only when a model fallback loads successfully.
- Added a memory-only thumbnail and failure cache with source-metadata revision IDs, serialized cache misses, and read-time path checks. The existing 8 MiB encoded-image and 16-million-pixel limits remain.
- Added deterministic image, fallback, and card-reuse regression tests plus a design-to-code review. Native mounted precedence and the game's default placeholder remain unverified. No game files or simulation state were changed.

## 0.11.1: Ratio editing beyond manual gearboxes

- Apply ratios and add or remove forward gears on manual, automatic, DCT, and sequential transmissions while keeping reverse and neutral.
- Re-engage the previously selected gear after resizing, so Apply no longer leaves the car revving in neutral.
- Support CVT as a single fixed-ratio edit. Reset restores the stock variator range when persistence is enabled.
- Show the detected transmission type and enable `+ Gear` only where gear-count editing is supported.
- Ratio edits still need a stopped, owned vehicle and remain target-bound and reset-aware.
- Clutch protection now also raises supported friction-clutch clamp capacity 20× so high-power tunes can transfer torque. Heat and wear stay frozen, but other drivetrain shock and damage remain possible. Reset restores the original clamp.

## 0.11.0: Latency, players, and presets

- Capture BeamMP's own local and remote ping updates without editing BeamMP files. Show provenance and freshness, and never treat `-1` as latency.
- Added stable session player rows with guest and session identity, role and vehicle counts, search and sort, public forum profile links, separate session-ID copying, account-ID lookup with cancellation guards, server address copying, convoy notes, and honest multiplayer policy text.
- Added limited ABM-owned presets that store accepted workshop overrides only. This includes validation, atomic writes, history, tags and notes, import and export, explicit apply preflight, stopped-own-vehicle requirements, partial-result reporting, and cancellation. Presets are not full native `.pc` clones.
- Added supported torque-curve and clutch readback, gear spread, seeded preview-first tune drafts, preview fallback labels, and a redacted support bundle.
- New controls need game bridge and vehicle bridge 0.11.0. Older bridges stay read-only where supported.
- Native BeamNG save and load pairing, real latency comparison, and physics effects remain unverified because the simulator and Lua bridge couldn't be connected during development.

## 0.10.0: Reset-recipe development

- Edit supported combustion-engine permanent RPM limits with actual idle and ceiling readback. Physical damage thresholds and temporary ESC and launch limiters are preserved. ABM doesn't synthesize torque curves.
- Protect supported friction clutches from additional thermal, wear, and deformation damage. Freeze their thermal update without increasing torque capacity or silently repairing an already damaged clutch.
- Keep accepted power, beam protection, strength, RPM, clutch, handling, manual gears, and experimental dimensions across ordinary resets of the same loaded vehicle. Restore once after readiness and authorization checks, then stop on failure instead of retrying a mutation loop.
- Added a per-vehicle persistence toggle and restoration status and error readback. Full reset and policy cleanup clear pending restoration before resetting physics. Recipes are memory-only, not automatic cross-vehicle presets.
- Show estimated RPM after an upshift beside actual and edited gear ratios. This is a ratio calculation, not a performance prediction.
- These additions need bridge and vehicle extension 0.10.0. Existing exact-name official-server restrictions remain.
- Native MCP discovery worked, but the running simulator couldn't be connected. Real-game reset, clutch, and geometry effects remain unverified.

## 0.9.0: Foundation development

- Separated live engine-power commands with limited latest-value scheduling, no beam or gravity edits, and finite-overflow checks.
- Added versioned vehicle-instance readback, engine RPM information, actual signed manual-gear ratios, and an editable per-gear overview.
- Suggested extra ratios use recent progression instead of a fixed 0.85 factor. Suggestions are not performance optima.
- Added target, session, generation, and expiry checks for new workshop commands, with cancellation on disconnect, policy loss, and vehicle-instance changes.
- Added a full reset at the bottom of Car. It cancels pending power, restores ABM vehicle adapters, and requests a stopped physics reset. World gravity is separate.
- This is roadmap slice A0, not the entire massive update. Editable RPM, clutch protection, and persistent reset recipes remained later work.

## 0.8.0: Private preview

- Added combinable Garage vehicle type, stock/mod/custom content, and favorites filters. Added brand, body style, configuration type, drivetrain, transmission, source/mod, country, fuel, and propulsion facets, plus six sort orders.
- Forward limited real game metadata and size-aware catalog chunks. Model facets cover any of their configurations. Unknown metadata is not guessed.
- Consolidated the workshop into four related sections. Read vehicle capability flags and disable unsupported, unsafe-at-speed, invalid, or policy-locked controls. Bounce stop stays available on restricted servers.
- Bridge protocol stays 0.7.1, and the official-server policy is unchanged. Capability discovery needs updated bridge files and a game restart.

## 0.7.1: Development snapshot

- Approve frozen regular official server names independently of IP and port, so host rotations work. Both receiver and game bridge use exact raw names. Old addresses don't restrict verified non-official replacements.
- Keep event and new official names blocked, and keep unknown classification locked. Current-connection leases still need fresh verification after a rotation.
- Multiplayer needs bridge 0.7.1. Name reuse and impersonation, including an event adopting an approved name, can't be distinguished by this policy.

## 0.7.0: Development snapshot

- Froze 108 regular official BeamMP identities (IP, port, and exact raw name), excluding the four MOZA qualifying servers. New or mismatched official servers are denied. Verified non-official servers remain unrestricted by this policy.
- Classify through BeamMP's public HTTPS directory, reject unverified servers, and enforce limited identity-bound leases in both receiver and game bridge.
- Lock gameplay actions, stop reapply, cancel pending commands, restore captured values, and request owned-car physics resets when permission or identity changes. Telemetry, browsing, and bounce stop stay available. Cleanup errors stay locked.
- Multiplayer mutations need bridge 0.7, and the fourth roster file is bundled. Added a visible lock reason and a blocked-server demo capture.
- This is not tamper-proof anti-cheat or server permission. Approved identities repurposed without changes can't be detected. Live transitions and cleanup remain unverified. See `docs/server-policy.md`.

## 0.6.2: Development snapshot

- Fixed the confirmed `selectShiftPoints` nil-entry crash beyond the factory top gear. The controller shift-point table now grows alongside the gearbox, synchro, and sound tables. Added gears inherit stock top-gear thresholds. Unsupported controller closures are rejected before mutation, reset restores stock, and inertia failures roll back.
- Block gear edits on pre-0.6.2 bridges, which still contain the crash.
- Allow stopped, owned-car resizing in BeamMP. Geometry is local and **not synchronized to other players**. Server rules still apply. No server bypass or remote vehicle edits. Real-game driving and peer behavior remain unverified.
- Corrected portable CI's Qt 6.8 architecture from `gcc_64` to `linux_gcc_64`. The previous job failed while installing Qt, before compilation. CI pending.

## 0.6.1: Development snapshot

- Added a separate Stop bouncing button using the real `vehicle:why("stop")` command. No reset, guessed toggle state, or repair is needed.
- Added independent 0.5-2× width, length, and height sliders. This is experimental physical-node and rest-length scaling that preserves captured preload and mass. It has stopped, single-player, and unbroken-beam guards. Reset restores stock. Controllers, tires, and props aren't rebuilt. Real-game behavior remains unverified.
- Both additions need bridge 0.6.1. Older action compatibility remains.
- Canonicalized Windows CLI-test temp and skill paths before checking containment, accommodating 8.3 aliases without weakening the isolated-HOME requirement. The preceding Windows run passed all Qt tests and failed this path assertion. The updated Windows job still needs to pass.

## 0.6.0: Development snapshot

- Added a 0.1-100× combustion-engine strength slider and an immediate protection toggle. Scale stock torque tolerance and over-torque and over-rev damage budgets without changing power or the rev limiter. Reset restores hooks and stock fields. Unsupported engines are rejected before any engine changes.
- Engine protection blocks the verified damage hooks, flooding, and over-rev and over-torque failures, and freezes thermal updates. It doesn't repair damage or protect the whole drivetrain. Real-game validation remains outstanding.
- Added a pinned-tool AppImage and bundled-archive recipe plus an Ubuntu, Debian, Fedora, and Arch startup matrix. It's experimental until CI passes, not a promise for all Linux.
- Documented researched sandbox expansions and their game API and ownership limits.
- Engine durability needs bridge 0.6. Older action compatibility remains.

## 0.5.0: Development snapshot

- Split the Car workshop into Power & world, Grip & handling, Gearbox, and Fun.
- Kept the power slider and added positive numeric percentages without a fixed cap. Reject non-finite input and derived torque overflow before mutation.
- Added static and sliding tire grip, brake strength, and steering-response multipliers, plus the supported vehicle ESC and traction-mode cycle. Damaged tires are skipped.
- Added editable forward ratios and an Add Gear button for verified manual gearboxes. There's no fixed gear-count cap, but a 24 KB serialized command budget applies. Synchro and sound tables grow together, and reset restores stock.
- Added the real `why("this dealer")` bounce command when the game exposes it.
- Report vehicle-VM completion and errors for new sandbox actions and tuning instead of treating a queued script as successful. Legacy fun actions still queue.
- Preserved 0.3 and 0.4 compatibility. New features need bridge 0.5.
- Real-game effects remain unverified. This doesn't change release v0.4.0.

## 0.4.0: 2026-11-09

First private GitHub preview release as **ABM: Advanced BeamNG.Drive Manager**.

### Included

- Native Qt Quick Basic dashboard: Drive, Car, Garage, and Session.
- Engine torque multiplier, world gravity slider and planet presets, reversible beam protection, repair, fire and extinguish, and tire deflation.
- Real installed model and configuration photos, favorites, custom configurations, safe spawn, replace, and switch confirmations, and ownership checks.
- Local telemetry receiver, trip estimates, speed and FPS history, and diagnostics.
- Steam and BeamMP launch controls, user-folder bridge installer, and Omarchy widget.
- Standalone receiver lifecycle and Windows Qt/MSVC deployment workflow.
- Extended installation, migration, user, CLI, architecture, security, troubleshooting, and development documentation.
- ABM branding for the binary, package, service, desktop entry, widget, and Windows release.

### Validation and boundaries

- 24 Qt tests plus Lua bridge, plugin, CLI, and standalone suites before the rename. All suites were rerun for the ABM release.
- Screenshot review of all pages, dialogs, menus, and the live bar popup.
- Windows runtime and real-game tuning effects aren't certified yet.
- Legacy wire, bridge, and environment identifiers are kept on purpose. See `docs/architecture.md`. There is no complete ECU editor or automatic updater.
