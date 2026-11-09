# Changelog

## 0.7.0 — Unreleased

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
