# ABM visual audit

## Unreleased — Session actions

- Inspected demo Session at 560×840 and 320×440, the owned-vehicle disabled-reason
  dialog, and the live-mode fixture's on-card forum-copy result. Evidence is in
  `/tmp/opencode/abm-session-review/`; fixture names/endpoints are synthetic.
  All three demo capture commands exited successfully without console warnings.
- Final `./bin/test`: **46 Qt checks passed**, plus Lua, Node, headless CLI and
  standalone lifecycle suites. The real-window Session test holds button presses
  across changing telemetry, clicks every player action (forum networking mocked),
  checks clipboard/browser dispatch, resets local trip statistics and confirms a
  vehicle-selection request. Copying is also exercised at the minimum viewport.
  No Session QML warnings; malformed-image fixtures intentionally warn elsewhere.
- No live BeamNG/BeamMP session or real browser/forum request was used. See
  [Session action review](session-actions.md) for documented contracts and limits.

## Unreleased — dashboard redesign

The default window is 560×840; the minimum remains 320×440. Compact windows use
tabs, wide windows (760 px onward) a sidebar. Short windows omit page headings;
the Garage's extra filters now live in a scrollable dialog. The custom
`MotorsportGlass` style identifier remains, with a new solid-surface appearance.

Reproduce a particular viewport without changing saved preferences:

```sh
QT_QUICK_BACKEND=software build/abm open --demo --tab garage --dev-size 1040x860 --screenshot /tmp/opencode/garage-wide.png
QT_QUICK_BACKEND=software build/abm open --demo --dev-open facets --dev-size 320x440 --screenshot /tmp/opencode/filters-small.png
```

`--dev-size` is accepted only with `--demo --screenshot`, from 320×440 to
2560×1600. The new layout test checks actual card placement (not just a column
count), gallery height with and without a selected model, filter-dialog bounds,
Escape dismissal and unchanged demo action restrictions at four viewport sizes.

- Inspected all four pages at 560×840, 320×440, 380×600 and 1040×860,
  plus configurations, Workshop groups, policy-locked controls, Help,
  diagnostics, setup, owned vehicles, trip/vehicle confirmations, menus,
  filters and the command palette. Extra compact-dialog checks cover setup,
  confirmations, filters, dimensions, palette and Help. Small pages scroll;
  the minimum-height configuration view intentionally shows only part of a card.
- Review fixes include actual multi-column placement, reclaiming short-window
  gallery space, keyboard-only focus outlines, dark menus and readable checkboxes.
  Confirmations name their action; replace/reset actions have a warning accent.
- Demo captures and logs: `/tmp/opencode/abm-dashboard-audit/`. All app capture
  commands exited successfully without console warnings. `docs/screenshot.png`
  now shows the wide Drive view with sample data, not private session data or
  bundled game artwork.
- Final `./bin/test`: **44 Qt checks passed**, plus Lua, Node, headless CLI and
  standalone lifecycle suites. Coverage includes real QML layout, keyboard
  menus/checkboxes/dialogs, preview reuse and existing action restrictions.
  Corrupt/missing-image fixtures intentionally emit decoder/image warnings.
- No installed app, game state or desktop configuration changed. Live-game,
  Windows, non-English and GPU-renderer visual checks were not performed.

## Unreleased — preview reliability

- Inspected demo Garage and configurations at 440×700, including loaded photos,
  missing previews and a correctly labeled custom configuration's model fallback.
  Captures: `/tmp/opencode/abm-preview-review/{garage,configs}.png`; neither
  command emitted console warnings. No game actions or live-game checks.
- `./bin/test` passed 43 Qt checks and the Lua, Node, CLI and standalone suites.
  New corruption fixtures intentionally emit decode/missing-image warnings.
  Card-reuse and broken-fallback transitions are covered by real QML tests.
- Tested working-tree code is based on `182458a`; only documentation changed
  afterward. See [design review](design-review.md) for reasoning and limits.

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
`settings-menu`, `filters`, `facets`, `palette`, `handling`, `gears`, `fun`, `dimensions`, `blocked`, and `engine`. These hooks require
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
