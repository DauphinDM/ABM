# Changelog

## 0.5.0 — Unreleased

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
