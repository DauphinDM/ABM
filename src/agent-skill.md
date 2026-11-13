---
name: abm
description: Inspect and troubleshoot ABM (Advanced BeamNG.Drive Manager), its telemetry receiver, and the Omarchy widget.
---

# ABM

Run `abm status --json` to see actual game telemetry. Exit 1 means disconnected or a receiver error. Never use demo readings as real data. Diagnose with `systemctl --user status abm.service` and `journalctl --user -u abm.service -n 30 --no-pager`.

`abm open` opens or focuses the app. Left-clicking the bar opens quick stats. Right-clicking opens the app. Vehicle actions need explicit confirmation in the GUI and a stationary vehicle. Never spawn, replace, or switch vehicles just to test connectivity. BeamMP server restrictions still apply.

`abm preferences --json` reads settings. Add `--units kmh` or `--units mph` to set units, or `--favorite MODEL` to toggle a favorite. These commands work without a display. Preferences are stored atomically in the app config folder.

Telemetry stays on localhost:44444. Runtime state is private under `$XDG_RUNTIME_DIR/abm/state.json`. Streaming and control use a user-only Unix socket beside it. The feed samples at up to 20 Hz, and gauges interpolate on a 60 Hz target frame clock. Missing values stay unavailable.

Car controls can change physics or damage the vehicle. Don't trigger them to diagnose connectivity. Get explicit user authorization and respect server rules. The legacy bridge ID stays for compatibility. After updating Lua, ask the user to run `extensions.unload("beamdeck"); extensions.load("beamdeck")` in the GE Lua console, or restart the game when convenient. Don't restart an active game without permission. Don't edit `/usr/share/omarchy/` or BeamMP's managed files.
