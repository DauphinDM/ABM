# ABM visual audit

## Unreleased: update controls

- The shared update controls start off and show the GitHub privacy disclosure, channel choice, verification status, and per-platform installation limits. The real QML card's Check now button is tested against a synthetic network manager, not public network traffic or a real installer.
- PC and Android host fixtures cover release and asset scope, hash and size rejection, redirect restrictions, cancellation, and preference persistence. Native iOS policy and preferences are tested separately in its simulator suite.
- The isolated 400×680 update-card capture (`/tmp/opencode/abm-updates-panel.png`) and normal demo Connect startup were inspected. Text and controls fit without QML warnings. Automated phone-size tests still load all pages. Physical installer handoff and optional Android release signing still need device or maintainer validation.

## Unreleased: mobile usability and USB

- Local PC suite: **52 Qt checks passed**, plus Lua, Node, headless CLI, and standalone lifecycle. New fixtures cover limited JPEG delivery and library deltas, remembered-token resume and revocation, empty-map spawning and multiplayer denial, non-destructive Android USB mappings, and pinned TLS through the PC iPhone byte relay. USB helpers and game responses are mocked. These tests don't change real device settings or game state.
- Mobile Qt checks load the real window, dismiss editor focus with Done, and verify that Apply sends the exact power command without an extra confirmation. They also exercise saved reconnects and a forty-thumbnail limited pipeline.
- The demo Connect transport selector was inspected at 560×840 without QML warnings. Native iOS checks now cover keyboard dismissal, thumbnail display, direct spawn and power actions, Keychain revocation, and the phone-side byte relay. These need the new Mac CI run before claiming a native pass.
- Earlier physical Wi-Fi pairing was confirmed by the user. The new USB mode, remembered-device workflow, and updated mobile features still need physical-device validation. Synthetic relay tests are not a latency benchmark.

## Unreleased: companion firewall setup

- Connect includes a visible explanation of a stalled secure connection, a supported-platform firewall setup button, and a separate scope and persistence confirmation before any administrator prompt. Demo mode disables this action.
- Pure rule tests check actual subnet calculation, exact interface, address, and TCP scope, rejection of unsupported or public or loopback or mask cases, PowerShell quoting, and UTF-16 encoding. Windows CI also parses the administrative script but never runs it. No automated test modifies a firewall.
- Real QML interaction checks opening, canceling, and accepting the confirmation, unchanged request count on cancellation, demo disabling, and limited dialog height at 320×440. The permission text scrolls at small sizes.
- Local full suite: **49 Qt checks passed**, plus Lua, Node, headless CLI, and standalone lifecycle. The demo Connect page and the 320×440 scope confirmation were inspected without QML warnings. All screenshot addresses are synthetic.
- On one local machine, kernel logs confirmed dropped phone SYNs to the listener. A separately user-approved, single-phone and single-interface TCP rule was added. That confirms the immediate blocker, not end-to-end pairing or Windows UAC behavior.

## Unreleased: phone connection foundation

- PC Connect uses the same scrollable panels and adaptive navigation as the dashboard. Demo mode disables networking and shows a synthetic private IP.
- The mobile repository has real QML checks for all five pages at 320×600, 412×860, and 800×600, without QML warnings. Its native-host tests exercise real TLS against a local fixture, certificate pin rejection, and stale-state shutdown.
- The PC TLS fixture covers explicit approval, snapshot redaction, revocation, replay rejection, and denial of stale, moving, unowned, or joining commands without a live game. Local PC suite: 47 Qt checks plus the existing Lua, Node, and Python suites.
- These checks don't prove Android device behavior or live multiplayer control. No phone listener was enabled on a real LAN during testing.

## Unreleased: Session actions

- Demo Session was inspected at 560×840 and 320×440, along with the owned-vehicle disabled-reason dialog and the live-mode fixture's on-card forum-copy result. Evidence is in `/tmp/opencode/abm-session-review/`. Fixture names and endpoints are synthetic. All three demo capture commands exited successfully without console warnings.
- Final `./bin/test`: **46 Qt checks passed**, plus Lua, Node, headless CLI, and standalone lifecycle suites. The real-window Session test holds button presses across changing telemetry, clicks every player action with forum networking mocked, checks clipboard and browser dispatch, resets local trip statistics, and confirms a vehicle-selection request. Copying is also exercised at the minimum viewport. There are no Session QML warnings. Malformed-image fixtures are expected to warn elsewhere.
- No live BeamNG/BeamMP session or real browser or forum request was used. See [Session action review](session-actions.md) for documented contracts and limits.

## Unreleased: dashboard redesign

The default window is 560×840. The minimum stays 320×440. Compact windows use tabs. Wide windows from 760 px use a sidebar. Short windows omit page headings. The Garage's extra filters now live in a scrollable dialog. The custom `MotorsportGlass` style identifier remains, with a new solid-surface appearance.

To reproduce a viewport without changing saved preferences:

```sh
QT_QUICK_BACKEND=software build/abm open --demo --tab garage --dev-size 1040x860 --screenshot /tmp/opencode/garage-wide.png
QT_QUICK_BACKEND=software build/abm open --demo --dev-open facets --dev-size 320x440 --screenshot /tmp/opencode/filters-small.png
```

`--dev-size` is accepted only with `--demo --screenshot`, from 320×440 to 2560×1600. The new layout test checks actual card placement, not just a column count. It also checks gallery height with and without a selected model, filter-dialog bounds, Escape dismissal, and unchanged demo action restrictions at four viewport sizes.

- All four pages were inspected at 560×840, 320×440, 380×600, and 1040×860. So were configurations, Workshop groups, policy-locked controls, Help, diagnostics, setup, owned vehicles, trip and vehicle confirmations, menus, filters, and the command palette. Extra compact-dialog checks cover setup, confirmations, filters, dimensions, the palette, and Help. Small pages scroll. The minimum-height configuration view is expected to show only part of a card.
- Review fixes include actual multi-column placement, reclaiming short-window gallery space, keyboard-only focus outlines, dark menus, and readable checkboxes. Confirmations name their action. Replace and reset actions have a warning accent.
- Demo captures and logs: `/tmp/opencode/abm-dashboard-audit/`. All app capture commands exited successfully without console warnings. `docs/screenshot.png` now shows the wide Drive view with sample data, not private session data or bundled game artwork.
- Final `./bin/test`: **44 Qt checks passed**, plus Lua, Node, headless CLI, and standalone lifecycle suites. Coverage includes real QML layout, keyboard menus, checkboxes and dialogs, preview reuse, and existing action restrictions. Corrupt and missing-image fixtures are expected to emit decoder and image warnings.
- No installed app, game state, or desktop configuration changed. Live-game, Windows, non-English, and GPU-renderer visual checks were not performed.

## Unreleased: preview reliability

- Demo Garage and configurations were inspected at 440×700, including loaded photos, missing previews, and a correctly labeled custom configuration's model fallback. Captures: `/tmp/opencode/abm-preview-review/{garage,configs}.png`. Neither command emitted console warnings. No game actions or live-game checks were performed.
- `./bin/test` passed 43 Qt checks plus the Lua, Node, CLI, and standalone suites. New corruption fixtures are expected to emit decode and missing-image warnings. Card reuse and broken-fallback transitions are covered by real QML tests.
- Tested working-tree code is based on `182458a`. Only documentation changed afterward. See [design review](design-review.md) for reasoning and limits.

## 0.8: selector facets and grouped workshop

- Ten demo captures were inspected in `/tmp/opencode/abm-08-audit`: four workshop groups, Garage, expanded facets, the content menu, configs, config facets, and policy-locked controls. The contact sheet is local, not a release asset.
- Review fixes: compact handling fields and editor, preserve space at narrow Garage widths, and show a clear placeholder for missing or corrupt previews. One installed game JPEG couldn't decode. The capture logs include that image-provider warning.
- Single test pass: 30 Qt tests passed, including combined filters and disabled moving, invalid, immunity, and policy controls. Node filter and security, headless CLI, and standalone lifecycle passed. Lua passed the new metadata and capability cases plus existing action and policy cases, then its isolated telemetry mock lacked `pcall`. The mock was fixed during review without a second test loop.
- Capability flags are conservative UI hints, not authorization. Updated bridge files need a game restart. Live capability reporting remains unverified. No live game commands or game restarts were performed.

## 0.7: official-server lock

- Allowed and blocked Car pages were captured in `/tmp/opencode/abm-07-audit` and reviewed at 440×700 with zero QML warnings. The lock reason wraps within the margins.
- Final-review fixes: hide misleading bridge-restart advice during a policy lock, dim the disabled immunity surface, preserve exact raw server identity, and start cleanup immediately when permission is lost, retrying failures later.
- One test pass: 28 Qt checks passed. One new test failed because it omitted required tuning fields before reaching the gateway. Its input was fixed, and the test wasn't rerun. Lua policy and cleanup tests, Node, headless CLI, and standalone lifecycle passed.
- Live server transitions and reset completion remain unverified. No live game commands or game restarts were performed during this audit.

## 0.6.2: extra-gear crash and multiplayer dimensions

- The real Pigeon failure was confirmed in `beamng.1.log`. Controller `selectShiftPoints` indexed missing gear 4, which disabled the vehicle.
- Single suite passed: 27 Qt tests, Lua bridge, plugin, CLI, and standalone lifecycle. Regression coverage includes controller lookups through 200 gears, unsupported or missing controllers, stale vehicle-module versions, inertia-error rollback, shrinking and reset, and own or foreign multiplayer resizing.
- Gearbox and Dimensions screenshots were inspected at 440×700 with zero QML warnings: `/tmp/opencode/abm-062-audit/`. The local-geometry multiplayer limitation is visible.
- Final review corrected portable CI's Qt 6.8 Linux architecture identifier. Portable and Windows CI plus live-game driving are not claimed verified.

## 0.6.1: bounce stop and physical dimensions

- Fun and Dimensions were captured and inspected at 440×700. Both fit with zero QML warnings. Local evidence: `/tmp/opencode/abm-061-audit/` (not release assets).
- One review run: 27 Qt tests passed. Lua reached and passed the new scaling, reset, ownership, and bounce-stop cases. It then failed an older repair assertion that assumed background tuning never interleaved. The assertion was fixed to check the actual repair call, and fire assertions now allow background tuning. No second full-suite run happened after the review fixes.
- Physical gameplay remains unverified. The experimental warning and consent plus the stopped and single-player gates are explicit. A Windows path-normalization fix awaits CI. There's no claim of a passing Windows build yet.

Demo captures are reproduced below. Private desktop evidence stays local and isn't committed. Rebranded demo screenshots are included in this directory.

Reviewed: Drive, Car, Garage, configurations, Session, Help, diagnostics, paths and setup, owned vehicles, trip reset, spawn, replace and switch confirmations, garage and options menus, the filter dropdown, and the live Omarchy popup.

Fixes include anchored menus, a legible help overlay, shorter diagnostics, populated setup fields, an owned-vehicles empty state, speed and FPS history, compact two-column Car actions, and clearer popup behavior without a vehicle.

To reproduce a page or dialog:

```sh
QT_QUICK_BACKEND=software build/abm open --demo --dev-open car --screenshot /tmp/opencode/car.png
QT_QUICK_BACKEND=software build/abm open --demo --dev-open diagnostics --screenshot /tmp/opencode/diagnostics.png
QT_QUICK_BACKEND=software build/abm open --demo --tab garage --pick etk800 --screenshot /tmp/opencode/configs.png
```

Capture targets: `drive`, `car`, `garage`, `session`, `help`, `diagnostics`, `paths`, `owned`, `trip`, `spawn`, `replace`, `switch`, `garage-menu`, `settings-menu`, `filters`, `facets`, `palette`, `handling`, `gears`, `fun`, `dimensions`, `blocked`, and `engine`. These hooks need demo screenshot mode.

Generated tuning Lua runs in tests to verify non-compounding torque scaling and restoration of beam limits. Actual in-game tuning effects still need verification after the updated bridge is loaded.

## Development 0.5 pass

19 demo views plus the live Omarchy popup were captured and reviewed. All demo captures produced zero QML warnings. The workshop is split into four compact sections instead of one long list. The final review moved the Options menu above its button with an inset, rejected empty handling inputs instead of treating them as zero, and kept **+ Gear** working at the minimum valid ratio. Windows tests now select the app's Basic control style instead of native controls, which had rejected customized backgrounds in the first Windows CI run.

Local evidence: `/tmp/opencode/abm-05-audit/` (not part of the repository). The full test pass happened before these final review fixes. There was no second review or test loop. In-game physics effects remain unverified.

## Development 0.6 pass

The new Engine strength section was captured and reviewed at the default 440×700 window. The slider, immediate protection toggle, apply and restore controls, and explicit thermal and coverage warnings fit without clipping. Zero QML warnings. Evidence: `/tmp/opencode/abm-06-audit/engine-flat.png` (local, not committed).

The single local suite passed 26 Qt tests plus Lua, plugin, CLI, and standalone lifecycle tests. VM mocks cover idempotent strength, protection on and off, original hook restoration, multi-engine preflight, and reset. This is not gameplay proof.
