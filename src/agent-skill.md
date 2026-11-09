---
name: abm
description: Inspect and troubleshoot ABM (Advanced BeamNG.Drive Manager), its telemetry receiver and Omarchy widget.
---

# ABM

Run `abm status --json` for actual game telemetry. Exit 1 means disconnected
or a receiver error; never substitute demo readings for real data. Diagnose with
`systemctl --user status abm.service` and
`journalctl --user -u abm.service -n 30 --no-pager`.

`abm open` opens or focuses the side app. The bar's left click opens quick
stats; right click opens the app. Vehicle actions require explicit confirmation
in the GUI and a stationary vehicle. Never spawn, replace or switch vehicles to
test connectivity. BeamMP server restrictions still apply.

`abm preferences --json` reads settings; add `--units kmh` or `--units mph`
to set units, or `--favorite MODEL` to toggle a favorite. These commands work
without a display. Preferences are atomically stored in the app config folder.

Telemetry stays on localhost:44444. State is private under
`$XDG_RUNTIME_DIR/abm/state.json`; streaming/control use a user-only Unix
socket beside it. The feed is sampled at up to 20 Hz and gauges interpolate on
a 60 Hz target frame clock. Missing values stay unavailable.

Car controls can alter physics or damage the vehicle. Do not trigger them to
diagnose connectivity. Use explicit user authorization and respect server rules.
The legacy bridge ID is retained for compatibility. After updating Lua, ask the user to run
`extensions.unload("beamdeck"); extensions.load("beamdeck")` in the GE Lua console
or restart the game when convenient. Do not restart an active game without
permission. Do not edit `/usr/share/omarchy/` or BeamMP's managed files.
