# ABM history: pre-release application audit and naming review

**Historical evidence, not current support instructions.** This predates the
ABM rebrand and later fixes; legacy names/paths are retained as audit evidence.
See the [current documentation](../../README.md) for release behavior.

Date: 2026-11-09. Scope: current source, local build/tests, package/integration scripts and read-only installed status. **No production fixes or rename have been applied by this audit.**

## Executive summary

The current development build is not ready to install. There are confirmed startup, configuration handling and control-channel regressions, in addition to the unfinished photographic garage and the cluttered layout. Correctness should precede visual polish and additional features.

The installed `beamdeck 0.2.0-1` is separate: its receiver is active, its packaged/user-folder Lua differs from the current source, and it reported disconnected during this audit. That is not evidence that the game crashed or is stopped; no game-process inference was made from that status. Nothing was restarted, spawned, replaced or switched in the live game.

## Tests actually run

| Check | Outcome |
| --- | --- |
| `bin/test` with software Qt Quick/offscreen Qt Test | Build succeeded; Qt: **18 passed, 1 failed**, including setup/cleanup. The real window cannot load. The script stops here, so later suites were run separately. |
| `luajit tests/game_test.lua` | **Failed**, line 70: the test still requests model-only spawning while the extension now demands model/config. Later Lua assertions were not reached. |
| `node tests/plugin_test.cjs` | Passed. This validates the JS model and selected source patterns, not actual popup interaction. |
| `python tests/cli_test.py` | Passed in an isolated HOME/runtime/config directory. |
| Six additional isolated backend/receiver behavior checks | **One behavior passed, five failed**. Qt also reports two passing setup/cleanup functions. Correct catalog loading was the passing control. See below. |
| QML/English translation-key scan | 17 QML references missing from the English catalog, including one repeated extension-button string. This is a reference count, not 17 unique strings or a full localization audit. |

Audit probes compile the production backend/receiver with a temporary harness under `/tmp/opencode/beamdeck-audit/`. They use temporary files and ephemeral localhost UDP ports, never the live port 44444. No production source was modified to make tests pass.

Evidence logs: `/tmp/opencode/beamdeck-audit-tests.log` and `/tmp/opencode/beamdeck-audit-probes.log`.

## Release blockers

### A01 — The new application window cannot open

**Confirmed by the real-QML test.** `src/VariantCard.qml:37` tries to instantiate `carThumb`, which is an object ID, not a component type. Qt reports `Cannot assign to non-existent property "carThumb"`, making `VariantCard` and therefore `Main.qml` unavailable.

Fix the component structure, then require the actual window to load without QML warnings. Do not mistake a successful C++ compilation for a usable app. Other QML problems below remain even after this first error is repaired.

### A02 — Successful config replies leave the picker empty

**Confirmed by an isolated round-trip check:** `backend.configs().size()` remains 0 after a successful response containing one configuration.

`src/backend.cpp:72–79` handles generic results before configuration-specific results. The first branch consumes the response and clears the pending action, so the later branch is unreachable.

Use one matching-result handler that processes its action/payload before clearing pending state. Assert actual returned configuration content, not just `busy == false` or a success message.

### A03 — Vehicle identity and spawn APIs disagree across layers

**Source-confirmed; existing Lua test also fails on the mismatch.**

- `src/backend.cpp:144–146` only permits a model identifier.
- `src/telemetry.cpp:297–300` requires `model/config`.
- `game/lua/ge/extensions/beamdeck.lua:154–168` requires `model/config`, validates only the model allowlist and passes the config key as a string instead of the expected options table.
- `beamdeck.lua:105` calls an undefined unqualified `getModelsData()`; config names/descriptions use lowercase fields rather than the installed game's `Configuration`/`Name`/`Description` fields.
- Config keys restricted to word characters/underscores/hyphens exclude real saved names with spaces and parentheses.

Carry model and config identity separately and validate against the game-resolved configuration catalog. Pass `{config = selectedConfig.pcFilename}` through the normal public spawn/replace APIs. Do not loosen arbitrary-path validation to accommodate custom names.

### A04 — The edited Lua networking no longer matches LuaSocket's API

**Source/API review; not live-tested in the game.** `beamdeck.lua:50–54,235–262` creates an unconnected socket but uses `send`/`receive` as if they accepted destination/source arguments. The unconnected APIs are `sendto`/`receivefrom`; alternatively restore the previously connected fixed localhost peer design. `commandSenderPort` is declared after functions referring to it, so those earlier functions refer to a global rather than that later local. Session-ID validation was also removed.

Restore a consistent localhost-only transport and current-session validation. The receiver sends through its bound `m_socket` (`telemetry.cpp:309`), not an invented separate ephemeral sender. Do not repeat the earlier unsubstantiated ephemeral-port diagnosis.

A direct system-LuaSocket experiment was unavailable because LuaJIT outside the game has no `socket` module installed; this was not treated as proof of game behavior. Add a strict socket contract mock, rather than the current mock that silently accepts any arguments.

### A05 — Reply origin and request correlation are not enforced

**Two isolated checks failed:** an unrelated localhost UDP socket could complete a pending request when given its ID, and an unsolicited catalog with an unknown ID replaced the catalog.

`src/telemetry.cpp:164–194` processes results/catalogs before the ordinary telemetry peer check. Results do not require a pending request; catalogs require only a nonempty ID.

Require the active peer port, active session, matching pending request and expected response type before updating state. Track request action/model, reject duplicates/replayed completions and validate/bound configuration fields. These are confirmed local protocol-validation flaws, **not a demonstrated internet-facing exploit**.

### A06 — Command results are delivered to unrelated clients

**Confirmed by an isolated observer socket:** it received another client's configuration result.

`telemetry.cpp:168–174` removes the requester entry and broadcasts the result to all clients. Take the requester pointer and send only to it. Explicit shared telemetry/catalog broadcasts are a separate concern. A second dashboard must not receive another request's private completion payload or use it to mutate selection.

### A07 — Commands can be forwarded after telemetry is declared disconnected

**Confirmed by an isolated check:** a spawn command was forwarded at a feed age of about 3.28 seconds, after `connected` became false. This was a mock game, not a real vehicle action.

`telemetry.cpp:245` expires the feed at 3 seconds; `telemetry.cpp:280` accepts commands until 10 seconds. Restore one consistent freshness policy, particularly for vehicle-changing actions, and revalidate session/ownership/motion in the game. Do not increase freshness timeouts to conceal invalid-input test failures.

## Functional and UX improvements

### A08 — The requested photographic garage still does not exist

`src/VariantCard.qml:16–27` draws a synthetic car; no preview data or image loader exists in the catalog path. `src/Main.qml:254–284` uses two lists instead of the requested model → configuration grids and explicitly says pictures remain in the game.

Implement the real preview pipeline and saved custom configs described in [the UX brief](ux-research.md). Missing pictures must not hide saved builds, and a model fallback photo must not masquerade as the exact custom configuration.

### A09 — Selection state cannot support reliable actions

- `Main.qml:279` compares `selectedModel + "/" + configKey` with `selectedModel`, which cannot identify a normal selected config.
- Lines 280–281 overwrite the model selection with a config ID, invalidating the variant filter's model-prefix comparison.
- `VariantCard.qml:41–42` references `canDriveAction` without a declared component input or owning ID.
- `Main.qml:242` reads `backend.pendingAction`, which is not exposed in `backend.h`.
- The Favorites-only filter remains, but there is no current garage control invoking `toggleFavorite`; the active-vehicle switcher described by the README is also absent from this QML.

Keep separate selected-model, selected-config and immutable confirmation-target state. Give cards explicit inputs/signals. Use one shared action bar; revalidate the frozen target on confirmation rather than using mutable browsing state.

### A10 — Launch/recovery controls are incomplete and misleading

- `Main.qml:128` shows the "BeamNG is not running" launch section only when `win.live` is true. Its enclosing `Item` also has no implicit/preferred height despite containing a layout.
- `Backend::startGame`/`startBeamMP` return booleans that QML ignores, so errors can appear to do nothing.
- `backend.cpp:155–161` hardcodes one home-folder launcher path, checks existence instead of executability, omits its working directory, does not activate the BeamMP mod and has no duplicate/startup-in-progress handling. The `--no-launch` flag itself is correct for the local launcher.
- `plugin/BeamdeckWidget.qml:55–58` only toggles the popup or opens the app; it neither starts a stopped game on click nor offers Start BeamMP.
- `gameSeen` means "ever received telemetry during this receiver lifetime" (`telemetry.cpp:200,249`), not "game process is currently present". `backend.h:41` also lacks a `Q_PROPERTY` for its accessor.
- Sending `loadExtension` to the missing extension cannot bootstrap that same extension. The transport needed to receive the command must already be running.

Model process presence, bridge connectivity, launch-in-progress and multiplayer readiness separately. Keep errors contextual and retryable. Put launching/mod setup in the GUI/helper's deliberate write-capable boundary, not the read-only receiver sandbox. Do not promise to fix an unloaded bridge by messaging it.

### A11 — The popup displays NaN when disconnected

`BeamdeckModel.parse` returns `{}` for disconnected/paused monitoring. `BeamdeckWidget.qml:84,87` checks `=== null` but not `undefined` before multiplying speed/fuel; it produces `NaN`. The local Omarchy `Commons/Numbers.js` formatter returns nonfinite values as strings. A standalone JS probe using the actual model and formatter reproduced **NaN km/h / NaN%**. Units also remain hardcoded km/h while the app supports mph.

Normalize unavailable fields consistently and reject nonfinite numbers before formatting. Show "—" or hide irrelevant rows, and share the units preference. Missing data must never look like an empty fuel tank.

### A12 — Too much permanent chrome, too many competing controls

`Main.qml` exposes a title/tagline, status card, four tabs, technical footer, repeated metrics and troubleshooting instructions. Garage contains two search fields, two competing lists and per-card actions. Tiny 9–11 px annotations do not solve information density.

Adopt [the community-informed minimal menu spec](community-research.md): Drive · Garage · Session, one context-sensitive search, one View selector, photo grid, visible favorite star and one shared action bar. Move diagnostic charts/frame rates and advanced vehicle telemetry into Details. Preserve functionality instead of deleting it to make screenshots look cleaner.

### A13 — Existing-window activation discards navigation requests

`main.cpp:87–90` activates an existing window and returns before parsing `--tab` at lines 114–118. Thus `beamdeck open --tab garage` does not navigate an already open app to Garage.

Validate the command first and send a small bounded activation message containing the requested destination. Keep it a fixed command schema, never executable code.

### A14 — Localization, window behavior and accessibility need another pass

- The missing English strings include launch, model/config searches and variant actions. Raw `spawn`/`replace` values are inserted into confirmation text rather than translated action labels.
- Main surfaces/text are hardcoded dark colors despite live accent support; full theme following is partial.
- `main.cpp:133–140` forcibly moves/resizes the window after opening, potentially fighting the compositor/user placement. Prefer a sensible initial default and remembered geometry, with multi-monitor bounds checks.
- The replacement confirmation uses generic OK rather than a target-specific action label; Escape closes the vehicle confirmation/help but not explicitly the trip dialog/search/back navigation.
- Gauge/chart canvas content needs meaningful accessibility summaries, not just visual curves. Actual contrast and screen-reader behavior were not measured in this audit.

Keep useful existing locale/theme foundations, add full-interaction tests and screenshot narrow/default/wide and translated layouts once startup works.

## Test, maintenance and packaging improvements

### A15 — Passing tests currently conceal broken behavior

`tests/beamdeck_tests.cpp:169–211` checks a result observed by an unrelated client, not the backend's actual `configs` contents, and explicitly expects a temporarily busy flag after invalid `../bad` input. Lines 117–123 create a control envelope and send it to the telemetry UDP endpoint, bypassing the real Unix command route; the resulting "invalid command" assertion does not test the gateway. The stale-config test never proves that it populated a config before clearing it, so it passes on an always-empty list.

`tests/game_test.lua` uses permissive socket/spawn mocks and lacks the installed game's config API/options shape. The real-window test is blocked by A01, then will encounter a missing `garageSearch` objectName (`beamdeck_tests.cpp:378` versus `Main.qml:246`). `bin/test:9` does not forward test arguments, so a requested test filter runs the entire suite. `bin/build` locks the app build, but test-directory qmake/make work is outside that lock.

Add real payload/selection assertions, strict mocks, wrong-peer/replay/stale-session tests and proper event-loop-driven line parsing. Never weaken production validation to turn a test green. Report suite failures individually and document supported focused-test invocation.

### A16 — Runtime and build/version boundaries are hard to identify

The source, binary, game bridge and plugin all say 0.2.0 while installed/source Lua differ. That made earlier debugging mix up shipped behavior with incomplete development changes. README claims about session validation and the current garage do not match this working tree.

Expose protocol version, bridge build and app build in secondary diagnostics; keep release metadata in one source of truth. Gate packaging on every suite. Update documentation/screenshots only from verified behavior. The Git status also shows all project files untracked; establish a clean baseline commit before a broad rename, without discarding any existing files.

### A17 — Installed integration upgrades depend on the source checkout

`pkgbuild/PKGBUILD:24–26` installs plugin/bridge templates, but `bin/enable` is not packaged as an installed setup command. It expects a source-tree layout. Upgrading the package alone therefore does not update the copied user-folder bridge/plugin files; re-running source `bin/enable` is required. It also copies files before plugin validation, so a failed validation can leave a partial update.

Provide a packaged, version-aware integration command with preflight validation, owned-file checks, atomic replacement/backups and clear partial-failure reporting. Keep custom-folder support and never overwrite unrelated files or restart the game automatically. External enable operations need timeouts and a useful failure message.

### A18 — Improve observability and structure without adding menu clutter

- Remove `[TG]` diagnostics and the `%d`/64-bit formatting warning at `telemetry.cpp:279` once fixes have reproducible tests. Structured, rate-limited error logging belongs in diagnostics, not continuous stderr noise.
- Check UDP write/JSON/file/settings failures instead of ignoring them. `Telemetry::errorString` currently reports only UDP errors even when Unix listen/directory setup failed.
- Chunk large config responses: Lua silently skips any encoded response over 32 KiB; 512 configs with names/descriptions can exceed that. Add completeness counts and bounded reassembly rather than silently losing the whole response.
- Chart entries contain no timestamps and turn absent speed/RPM/FPS into zero (`backend.cpp:49–51`, `TrendChart.qml:25–30`). Use actual sample times and gaps for unknown values; a 300-entry buffer sampled with jitter is not necessarily the advertised last 30 seconds.
- Frame-clock smoothing runs while any tab is selected; hidden charts still receive history updates. Profile first, then suspend offscreen work where useful. No performance benchmark or measured regression is claimed here.
- Keep a flat source tree but separate launch/discovery work from telemetry and catalog/image work as those grow. Centralize protocol validation and use clear state types rather than scattered string actions and booleans. Do not introduce an elaborate plugin framework just to tidy the code.

## Strengths to preserve

- Private runtime snapshots, user-only Unix sockets, localhost receiver binding and a sandboxed receiver service.
- Plain-text treatment of game/server/player strings; no arbitrary Lua execution command.
- Temporary test paths and demo mode intended to avoid live-game mutations.
- Game-side ownership/movement checks and normal BeamMP API overrides.
- Bounded telemetry/history, atomic snapshot writes, live accent/language foundations and installer backups.

The right outcome is repairing and simplifying this design, not replacing it with a larger architecture.

## Naming recommendation

**Recommendation: Beamside** — clear connection to BeamNG and the side-panel role, easy to read in the bar, and no "deck"/Stream Deck implication. Treat this as a proposed name, not a rename already carried out.

Alternatives:

- **Pitglass:** more distinctive; suggests a glanceable cockpit/garage window. Less immediately descriptive, and searches show non-software automotive glass businesses using the word.
- **Paddock:** natural garage/session language, but crowded within sim racing (for example Paddock Life/Virtual Paddock), so less attractive for discoverability.

There is an actual naming collision with [Eagle10021/BeamDeck](https://github.com/Eagle10021/BeamDeck), a different Linux/Windows BeamNG/BeamMP media companion. The name is also associated with [a BeamNG Stream Deck icon pack](https://www.beamng.com/threads/beam-deck-beamng-icon-pack-for-elgato-stream-deck-and-sim-rigs-1735-icons.87570/). Preliminary searches also found Pitlane telemetry products and a BeamLine mod creator, so those were not selected.

This was a preliminary search, **not trademark, domain or package-name clearance**. Search absence does not establish uniqueness. Choose the public name before changing identifiers.

### Rename safely after the name is chosen

1. Define display name, lowercase executable/package ID and plugin ID together.
2. Account for the desktop file/icon/app ID, service/runtime socket paths, plugin manifest/IPC target, game extension filename/startup loader and hardcoded vehicle-Lua callback name.
3. Preserve and migrate favorites/units and other user settings with ownership checks and backups. Do not delete the old config directory or blindly perform a global replacement in user files.
4. Make the package transition explicit; avoid two receivers/services loading two game extensions. Old-name compatibility must be deliberate, not duplicate startup.
5. Update any Beamdeck-specific compositor rules only with the applicable desktop-configuration workflow and user-owned-file checks.
6. Leave the live game untouched; the new extension identity takes effect on a deliberate reload or next launch. Validate migration and rollback before installation.

## Proposed order of work

1. Repair startup, transport, request validation and freshness; add the failing audit cases as regression tests.
2. Deliver the real model → config photo grid with custom configs and exact selected-config actions.
3. Apply the minimal navigation, honest empty states, shared action bar and contextual diagnostics.
4. Finish launch/mod setup and read-only live verification without interrupting an active game.
5. Apply the chosen name with migration, package a new identifiable version, install and verify.

Do not spend the next iteration adding graphs, assistants, collections or tuning controls before these fundamentals work.
