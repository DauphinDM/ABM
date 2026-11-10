# ABM visual audit

## 0.8 — selector facets and grouped workshop

- Inspected ten demo captures in `/tmp/opencode/abm-08-audit`: four workshop
  groups, Garage, expanded facets, content menu, configs, config facets and
  policy-locked controls. Contact sheet is local, not a release asset.
- Review fixes: compact handling fields/editor, preserve space at narrow Garage
  widths, and show a clear placeholder for missing/corrupt previews. One installed
  game JPEG could not decode; capture logs include that image-provider warning.
- Single test pass: 30 Qt tests passed, including combined filters and disabled
  moving/invalid/immunity/policy controls. Node filter/security, headless CLI and
  standalone lifecycle passed. Lua passed new metadata/capability and existing
  action/policy cases, then its isolated telemetry mock lacked `pcall`; fixed
  the mock during review, without a second test loop.
- Capability flags are conservative UI hints, not authorization. Updated bridge
  files require a game restart; live capability reporting remains unverified.
  No live game commands or game restarts were performed.

## 0.7 — official-server lock

- Captured allowed and blocked Car pages in `/tmp/opencode/abm-07-audit`,
  reviewed at 440×700; zero QML warnings. The lock reason wraps within margins.
- Final-review fixes: hide misleading bridge-restart advice during a policy
  lock, dim the disabled immunity surface, preserve exact raw server identity,
  and initiate cleanup immediately on permission loss (retry failures later).
- One test pass: 28 Qt checks passed and one new test failed because it omitted
  required tuning fields before reaching the gateway. Fixed its input; no repeat
  run. Lua policy/cleanup tests, Node, headless CLI and standalone lifecycle passed.
- Live server transitions/reset completion remain unverified. No live game
  commands or game restarts were performed during this audit.

## 0.6.2 — extra-gear crash and multiplayer dimensions

- Confirmed the real Pigeon failure in `beamng.1.log`: controller
  `selectShiftPoints` indexed missing gear 4, disabling the vehicle.
- Single suite passed: 27 Qt tests, Lua bridge, plugin, CLI and standalone
  lifecycle. Regression coverage includes controller lookups through 200 gears,
  unsupported/missing controllers, stale vehicle-module versions, inertia-error
  rollback, shrinking/reset and own/foreign multiplayer resizing.
- Inspected Gearbox and Dimensions screenshots at 440×700, zero QML warnings:
  `/tmp/opencode/abm-062-audit/`. Local-geometry multiplayer limitation is visible.
- Final review corrected portable CI's Qt 6.8 Linux architecture identifier;
  portable/Windows CI and live-game driving are not claimed verified.

## 0.6.1 — bounce stop and physical dimensions

- Captured and inspected Fun and Dimensions at 440×700; both fit, zero QML
  warnings. Local evidence: `/tmp/opencode/abm-061-audit/` (not release assets).
- One review run: 27 Qt tests passed. Lua reached/passed the new scaling,
  reset, ownership and bounce-stop cases, then failed an older repair assertion
  that assumed background tuning never interleaved. Fixed that assertion to
  check the actual repair call, and fire assertions to allow background tuning.
  No second full-suite run after the review fixes.
- Physical gameplay remains unverified; experimental warning/consent and
  stopped/single-player gates are explicit. Windows path-normalization fix
  awaits CI; no claim of a passing Windows build yet.

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
`settings-menu`, `filters`, `handling`, `gears`, `fun`, and `engine`. These hooks require
demo screenshot mode.

Generated tuning Lua is executed in tests to verify non-compounding torque
scaling and restoration of beam limits. Actual in-game tuning effects still
need verification after the updated bridge is loaded.

## Development 0.5 pass

Captured and reviewed 19 demo views, plus the live Omarchy popup. All demo
captures produced zero QML warnings. The workshop is split into four compact
sections rather than one long list. The final review moved the Options menu
above its button with an inset, rejected empty handling inputs instead of
treating them as zero, and kept **+ Gear** working at the minimum valid ratio.
Windows tests now select the app's Basic control style instead of native
controls, which rejected customized backgrounds in the first Windows CI run.

Local evidence: `/tmp/opencode/abm-05-audit/` (not part of the repository).
The full test pass preceded these final review fixes; there was no second
review/test loop. In-game physics effects remain unverified.

## Development 0.6 pass

Captured and reviewed the new Engine strength section at the default 440×700
window: slider, immediate protection toggle, apply/restore controls and explicit
thermal/coverage warnings fit without clipping. Zero QML warnings. Evidence:
`/tmp/opencode/abm-06-audit/engine-flat.png` (local, not committed).

The single local suite passed 26 Qt tests plus Lua, plugin, CLI and standalone
lifecycle tests. VM mocks cover idempotent strength, protection on/off, original
hook restoration, multi-engine preflight and reset. This is not gameplay proof.
