# ABM history: pre-release application audit and naming review

**Historical evidence, not current support instructions.** This predates the ABM
rebrand and later fixes, so legacy names and paths stay as audit evidence. See
the [current documentation](../../README.md) for release behavior.

Date: 2026-11-09. Scope: the source at the time, local build and tests,
package/integration scripts and read-only installed status. This audit applied
no production fixes and no rename.

## Executive summary

The development build from that day wasn't ready to install. We confirmed
regressions in startup, configuration handling and the control channel. The
photographic garage was still unfinished, and the layout was cluttered. Fix
correctness before visual polish or new features.

The installed `beamdeck 0.2.0-1` is a separate thing. Its receiver was active,
its packaged and user-folder Lua differed from the current source, and it
reported disconnected during the audit. That doesn't mean the game crashed or
was stopped. We didn't infer anything about the game process from that status,
and we didn't restart, spawn, replace or switch anything in the live game.

## Tests actually run

| Check | Outcome |
| --- | --- |
| `bin/test` with software Qt Quick/offscreen Qt Test | Build succeeded. Qt: **18 passed, 1 failed**, including setup and cleanup. The real window couldn't load. The script stops there, so we ran the later suites separately. |
| `luajit tests/game_test.lua` | **Failed** at line 70. The test still asks for model-only spawning while the extension now requires model/config. Later Lua assertions weren't reached. |
| `node tests/plugin_test.cjs` | Passed. This checks the JS model and selected source patterns, not actual popup interaction. |
| `python tests/cli_test.py` | Passed in an isolated HOME, runtime and config directory. |
| Six additional isolated backend/receiver behavior checks | **One behavior passed, five failed**. Qt also reports two passing setup and cleanup functions. Correct catalog loading was the passing control. See below. |
| QML/English translation-key scan | 17 QML references missing from the English catalog, including one repeated extension-button string. That's a reference count, not 17 unique strings, and not a full localization audit. |

The audit probes compiled the production backend and receiver with a temporary
harness under `/tmp/opencode/beamdeck-audit/`. They used temporary files and
ephemeral localhost UDP ports, never the live port 44444. We didn't change
production source to make tests pass.

Evidence logs: `/tmp/opencode/beamdeck-audit-tests.log` and
`/tmp/opencode/beamdeck-audit-probes.log`.

## Release blockers

### A01: The new application window cannot open

The real-QML test confirmed this. `src/VariantCard.qml:37` tries to instantiate
`carThumb`, but that's an object ID, not a component type. Qt reports `Cannot
assign to non-existent property "carThumb"`, so `VariantCard` and `Main.qml`
fail to load.

Fix the component structure, then require the real window to load without QML
warnings. A successful C++ build doesn't prove you have a usable app. The other
QML problems below will still be there after you fix this first error.

### A02: Successful config replies leave the picker empty

An isolated round-trip check confirmed it. `backend.configs().size()` stayed 0
after a successful response that contained one configuration.

At `src/backend.cpp:72-79`, the generic result handler runs before the
configuration-specific one. The first branch consumes the response and clears
the pending action, so the later branch never runs.

Use one handler for matching results. It should process the action and payload
before clearing pending state. In tests, assert the actual configuration
content, not only `busy == false` or a success message.

### A03: Vehicle identity and spawn APIs disagree across layers

Source review confirmed this, and the existing Lua test fails on the same
mismatch.

- `src/backend.cpp:144-146` accepts only a model identifier.
- `src/telemetry.cpp:297-300` requires `model/config`.
- `game/lua/ge/extensions/beamdeck.lua:154-168` also requires `model/config`,
  validates only the model allowlist, and passes the config key as a string
  instead of the options table the API expects.
- `beamdeck.lua:105` calls `getModelsData()` without a qualifier, and that
  function isn't defined. Config names and descriptions use lowercase fields
  instead of the installed game's `Configuration`, `Name` and `Description`
  fields.
- Config-key validation only allows word characters, underscores and hyphens.
  That rejects real saved names with spaces and parentheses.

Keep model and config identity separate, and validate against the configuration
catalog the game resolves. Pass `{config = selectedConfig.pcFilename}` through
the normal public spawn/replace APIs. Don't weaken path validation just so
arbitrary paths work with custom names.

### A04: The edited Lua networking no longer matches LuaSocket's API

This came from source and API review, not a live game test.
`beamdeck.lua:50-54,235-262` creates an unconnected socket but calls `send` and
`receive` as if they took destination and source arguments. For an unconnected
socket, the APIs are `sendto` and `receivefrom`. The other option is to restore
the earlier design with a connected fixed localhost peer. `commandSenderPort` is
declared after the functions that use it, so those functions see a global
instead of the later local. Session-ID validation was removed too.

Restore one consistent localhost-only transport and keep current-session
validation. The receiver sends through its bound `m_socket`
(`telemetry.cpp:309`), not a separate ephemeral sender that doesn't exist.
Don't repeat the earlier ephemeral-port diagnosis; nothing supported it.

We couldn't run a direct system-LuaSocket experiment because LuaJIT outside the
game has no `socket` module installed. We didn't treat that as proof of how the
game behaves. Add a strict socket-contract mock; the current mock silently
accepts any arguments.

### A05: Reply origin and request correlation are not enforced

Two isolated checks failed. An unrelated localhost UDP socket completed a
pending request when it was given the ID, and an unsolicited catalog with an
unknown ID replaced the catalog.

`src/telemetry.cpp:164-194` processes results and catalogs before the usual
telemetry peer check. Results don't require a pending request, and catalogs
only require a nonempty ID.

Before updating state, require the active peer port, the active session, a
matching pending request and the expected response type. Track the request
action and model, reject duplicate or replayed completions, and validate
configuration fields and cap their size. These are confirmed local
protocol-validation flaws, **not a demonstrated internet-facing exploit**.

### A06: Command results are delivered to unrelated clients

An isolated observer socket confirmed it. The socket received another client's
configuration result.

`telemetry.cpp:168-174` removes the requester entry and broadcasts the result
to every client. Take the requester pointer and send the result only to that
client. Shared telemetry and catalog broadcasts are a separate concern. A second
dashboard must not receive another request's private completion payload or use
it to change its selection.

### A07: Commands can be forwarded after telemetry is declared disconnected

An isolated check confirmed it. A spawn command was forwarded when the feed was
about 3.28 seconds old, after `connected` had become false. This was a mock
game, not a real vehicle action.

`telemetry.cpp:245` expires the feed at 3 seconds. `telemetry.cpp:280` accepts
commands until 10 seconds. Restore one consistent freshness policy, especially
for actions that change the vehicle. Recheck session, ownership and motion in
the game. Don't raise freshness timeouts to hide invalid-input test failures.

## Functional and UX improvements

### A08: The requested photographic garage still doesn't exist

`src/VariantCard.qml:16-27` draws a synthetic car. The catalog path has no
preview data and no image loader. `src/Main.qml:254-284` uses two lists instead
of the model → configuration grids we asked for, and the text says pictures
stay in the game.

Build the real preview pipeline and saved custom configs described in [the UX
brief](ux-research.md). Missing pictures must not hide saved builds. A fallback
model photo must not pass as the exact custom configuration.

### A09: Selection state cannot support reliable actions

- `Main.qml:279` compares `selectedModel + "/" + configKey` with
  `selectedModel`. That can't identify a normal selected config.
- Lines 280-281 overwrite the model selection with a config ID, which breaks
  the variant filter's model-prefix comparison.
- `VariantCard.qml:41-42` uses `canDriveAction` without a declared component
  input or owning ID.
- `Main.qml:242` reads `backend.pendingAction`, but `backend.h` doesn't expose
  it.
- The Favorites-only filter is still there, but no garage control calls
  `toggleFavorite`. The active-vehicle switcher the README describes is also
  missing from this QML.

Keep selected model, selected config and the confirmation target in separate
state, and don't change the target while confirmation is open. Give cards clear
inputs and signals. Use one shared action bar, and revalidate the frozen target
when the user confirms instead of reading mutable browsing state.

### A10: Launch/recovery controls are incomplete and misleading

- `Main.qml:128` shows the "BeamNG is not running" launch section only when
  `win.live` is true. The `Item` around it has no implicit or preferred height
  even though it holds a layout.
- `Backend::startGame` and `startBeamMP` return booleans that QML ignores, so a
  failure can look like nothing happened.
- `backend.cpp:155-161` hardcodes one home-folder launcher path. It checks that
  the file exists but not that it can execute, leaves out the working directory,
  doesn't activate the BeamMP mod, and has no handling for duplicate starts or
  a start already in progress. The `--no-launch` flag itself is correct for the
  local launcher.
- `plugin/BeamdeckWidget.qml:55-58` only toggles the popup or opens the app.
  Clicking it doesn't start a stopped game, and it doesn't offer Start BeamMP.
- `gameSeen` means "received telemetry at some point during this receiver's
  lifetime" (`telemetry.cpp:200,249`). It does not mean the game process is
  running now. `backend.h:41` also lacks a `Q_PROPERTY` for its accessor.
- Sending `loadExtension` to an extension that isn't loaded can't bootstrap that
  extension. The transport that receives the command has to be running already.

Track process presence, bridge connectivity, launch in progress and multiplayer
readiness as separate states. Keep errors tied to the action and make them
retryable. Put launching and mod setup in the GUI or helper process, which is
allowed to write, not in the read-only receiver sandbox. Don't promise to fix an
unloaded bridge by sending it a message.

### A11: The popup displays NaN when disconnected

`BeamdeckModel.parse` returns `{}` when monitoring is disconnected or paused.
`BeamdeckWidget.qml:84,87` checks `=== null` but not `undefined` before
multiplying speed and fuel, so you get `NaN`. The local Omarchy
`Commons/Numbers.js` formatter returns nonfinite values as strings. A standalone
JS probe with the real model and formatter reproduced **NaN km/h / NaN%**. Units
are also hardcoded to km/h even though the app supports mph.

Normalize unavailable fields the same way everywhere, and reject nonfinite
numbers before formatting. Show a dash or hide rows that don't apply, and use
the shared units preference. Missing data must never look like an empty fuel
tank.

### A12: Too much permanent chrome, too many competing controls

`Main.qml` has a title and tagline, a status card, four tabs, a technical
footer, repeated metrics and troubleshooting instructions. Garage has two
search fields, two competing lists and actions on every card. Tiny 9-11 px
annotations don't fix information density.

Use [the community-informed minimal menu spec](community-research.md): Drive,
Garage, Session, one search for the current level, one View selector, a photo
grid, a visible favorite star and one shared action bar. Move diagnostic charts,
frame rates and advanced vehicle telemetry into Details. Keep the
functionality; don't delete it just to make screenshots look cleaner.

### A13: Existing-window activation discards navigation requests

`main.cpp:87-90` activates an existing window and returns before it parses
`--tab` at lines 114-118. So `beamdeck open --tab garage` doesn't move an
already open app to Garage.

Validate the command first, then send a small activation message with the
requested destination. Cap its size. Keep it to a fixed command schema; never
send executable code.

### A14: Localization, window behavior and accessibility need another pass

- The missing English strings cover launch, model and config searches, and
  variant actions. Raw `spawn` and `replace` values go into confirmation text
  instead of translated action labels.
- Main surfaces and text use hardcoded dark colors even though live accent
  support exists. Full theme following is only partial.
- `main.cpp:133-140` force-moves and resizes the window after opening, which can
  fight the compositor or the user's placement. Start with a sensible default
  and remember geometry, with bounds checks for multiple monitors.
- The replacement confirmation uses a generic OK instead of an action label for
  the target. Escape closes the vehicle confirmation and help, but not the trip
  dialog, search or back navigation.
- Gauge and chart canvas content needs useful accessibility summaries, not only
  visual curves. We didn't measure actual contrast or screen-reader behavior in
  this audit.

Keep the useful locale and theme foundations. Add full-interaction tests and
take screenshots at narrow, default and wide sizes plus translated layouts once
startup works.

## Test, maintenance and packaging improvements

### A15: Passing tests currently conceal broken behavior

`tests/beamdeck_tests.cpp:169-211` checks a result seen by an unrelated client,
not the backend's actual `configs` contents. It also expects a temporarily busy
flag after invalid `../bad` input. Lines 117-123 create a control envelope and
send it to the telemetry UDP endpoint, which bypasses the real Unix command
route. The resulting "invalid command" assertion doesn't test the gateway. The
stale-config test never shows that it populated a config before clearing it, so
it passes on a list that's always empty.

`tests/game_test.lua` uses loose socket and spawn mocks and doesn't match the
installed game's config API or options shape. A01 blocks the real-window test.
Once that's fixed, the test will hit a missing `garageSearch` objectName
(`beamdeck_tests.cpp:378` versus `Main.qml:246`). `bin/test:9` doesn't forward
test arguments, so asking for one filter runs the whole suite. `bin/build` locks
the app build, but qmake and make work in the test directory happens outside
that lock.

Add assertions for real payloads and selection, strict mocks, tests for wrong
peer, replay and stale session, and line parsing driven by the event loop.
Never weaken production validation to make a test pass. Report each suite's
failures separately and document how to run a focused test.

### A16: Runtime and build/version boundaries are hard to identify

The source, binary, game bridge and plugin all said 0.2.0, but the installed
Lua and source Lua differed. Earlier debugging mixed up shipped behavior with
unfinished development changes because of that. README claims about session
validation and the current garage didn't match this working tree.

Show the protocol version, bridge build and app build in secondary diagnostics.
Keep release metadata in one place. Make packaging wait on every suite. Update
docs and screenshots only from behavior you've verified. Git status also showed
every project file untracked. Make a clean baseline commit before a broad
rename, and don't discard any existing files.

### A17: Installed integration upgrades depend on the source checkout

`pkgbuild/PKGBUILD:24-26` installs plugin and bridge templates, but the package
doesn't include `bin/enable` as an installed setup command. It expects a
source-tree layout. Upgrading the package alone won't update the copied
user-folder bridge and plugin files; you have to rerun `bin/enable` from the
source tree. It also copies files before plugin validation, so a failed
validation can leave a partial update.

Ship a packaged integration command that knows about versions, runs preflight
validation, checks file ownership, replaces files atomically with backups, and
reports partial failure clearly. Keep custom-folder support. Never overwrite
unrelated files or restart the game on its own. External enable operations need
timeouts and a failure message that helps.

### A18: Improve observability and structure without adding menu clutter

- Remove the `[TG]` diagnostics and the `%d`/64-bit formatting warning at
  `telemetry.cpp:279` after the fixes have reproducible tests. Structured,
  rate-limited error logging belongs in diagnostics, not continuous stderr
  noise.
- Check UDP write, JSON, file and settings failures instead of ignoring them.
  `Telemetry::errorString` reports only UDP errors right now, even when Unix
  listen or directory setup failed.
- Chunk large config responses. Lua silently skips any encoded response over
  32 KiB, and 512 configs with names and descriptions can go over that. Add
  completeness counts and reassemble with a size cap so you don't lose the
  whole response without a warning.
- Chart entries have no timestamps and turn missing speed, RPM or FPS into zero
  (`backend.cpp:49-51`, `TrendChart.qml:25-30`). Use real sample times and gaps
  for unknown values. A 300-entry buffer sampled with jitter isn't necessarily
  the advertised last 30 seconds.
- Frame-clock smoothing runs while any tab is selected, and hidden charts still
  get history updates. Profile first, then suspend offscreen work where it
  helps. This audit claims no performance benchmark or measured regression.
- Keep the flat source tree, but separate launch and discovery work from
  telemetry and catalog/image work as they grow. Centralize protocol validation
  and use clear state types instead of scattered string actions and booleans.
  Don't add an elaborate plugin framework just to tidy the code.

## Strengths to preserve

- Private runtime snapshots, Unix sockets only your user can access, a receiver
  bound to localhost and a sandboxed receiver service.
- Plain-text treatment of game, server and player strings; no arbitrary Lua
  execution command.
- Temporary test paths and demo mode meant to avoid live-game mutations.
- Game-side ownership and movement checks, plus normal BeamMP API overrides.
- Telemetry and history with size caps, atomic snapshot writes, live accent and
  language foundations, and installer backups.

The right outcome is to repair and simplify this design, not replace it with a
bigger architecture.

## Naming recommendation

**Recommendation: Beamside.** It connects clearly to BeamNG and the side-panel
role, it's easy to read in the bar, and it doesn't imply "deck" or Stream Deck.
Treat this as a proposed name, not a rename we've already done.

Alternatives:

- **Pitglass:** more distinctive, and it suggests a cockpit or garage window you
  can glance at. It's less immediately descriptive, and searches turn up
  non-software automotive glass businesses using the name.
- **Paddock:** natural garage and session language, but crowded in sim racing
  (Paddock Life and Virtual Paddock, for example), so it's less attractive for
  discoverability.

The name Beamdeck has a real collision with
[Eagle10021/BeamDeck](https://github.com/Eagle10021/BeamDeck), a different Linux
and Windows BeamNG/BeamMP media companion. It's also tied to [a BeamNG Stream
Deck icon pack](https://www.beamng.com/threads/beam-deck-beamng-icon-pack-for-elgato-stream-deck-and-sim-rigs-1735-icons.87570/).
Preliminary searches found Pitlane telemetry products and a BeamLine mod
creator, so we didn't pick those.

This was a preliminary search, not trademark, domain or package-name clearance.
Not finding a name in search doesn't make it unique. Choose the public name
before you change identifiers.

### Rename safely after the name is chosen

1. Decide the display name, the lowercase executable and package ID, and the
   plugin ID together.
2. Cover the desktop file, icon and app ID; service and runtime socket paths;
   plugin manifest and IPC target; game extension filename and startup loader;
   and the hardcoded vehicle-Lua callback name.
3. Keep and migrate favorites, units and other user settings, with ownership
   checks and backups. Don't delete the old config directory, and don't do a
   global replacement across user files.
4. Spell out the package transition, and avoid two receivers or services
   loading two game extensions. Old-name compatibility should be a planned
   step, not duplicate startup.
5. Update Beamdeck-specific compositor rules only through the desktop's
   configuration workflow, with checks for user-owned files.
6. Leave the live game alone. The new extension identity takes effect after a
   reload you choose or on the next launch. Validate migration and rollback
   before installation.

## Proposed order of work

1. Fix startup, transport, request validation and freshness, and add the
   failing audit cases as regression tests.
2. Build the real model → config photo grid with custom configs and actions for
   the exact selected config.
3. Apply the minimal navigation, empty states that explain what's missing, a
   shared action bar and contextual diagnostics.
4. Finish launch and mod setup, then verify against the live game in read-only
   mode without interrupting it.
5. Apply the chosen name with migration, package a new version you can
   identify, install it and verify.

Don't spend the next iteration on graphs, assistants, collections or tuning
controls until these basics work.