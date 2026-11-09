# ABM 0.4 visual audit

Demo captures are reproduced below; private desktop evidence is kept locally,
not committed. Rebranded demo screenshots are included in this directory.

Reviewed Drive, Car, Garage, configurations, Session, Help, diagnostics,
paths/setup, owned vehicles, trip reset, spawn/replace/switch confirmations,
garage/options menus, filter dropdown, and the live Omarchy popup.

Fixes include anchored menus, a legible help overlay, shorter diagnostics,
populated setup fields, an owned-vehicles empty state, speed/FPS history,
compact two-column Car actions, and clearer popup behavior without a vehicle.

Reproduce a page or dialog:

```sh
QT_QUICK_BACKEND=software build/abm open --demo --dev-open car --screenshot /tmp/opencode/car.png
QT_QUICK_BACKEND=software build/abm open --demo --dev-open diagnostics --screenshot /tmp/opencode/diagnostics.png
QT_QUICK_BACKEND=software build/abm open --demo --tab garage --pick etk800 --screenshot /tmp/opencode/configs.png
```

Capture targets: `drive`, `car`, `garage`, `session`, `help`, `diagnostics`,
`paths`, `owned`, `trip`, `spawn`, `replace`, `switch`, `garage-menu`,
`settings-menu`, and `filters`. These hooks require demo screenshot mode.

Generated tuning Lua is executed in tests to verify non-compounding torque
scaling and restoration of beam limits. Actual in-game tuning effects still
need verification after the updated bridge is loaded.
