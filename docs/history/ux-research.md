# ABM history: early UX research and redesign brief

**Archived pre-release design evidence.** Older names and proposals below are
historical, not the current ABM feature list. See [ABM](../../README.md).

Research date: 2026-11-09. Status: proposal and source audit. This is **not an
implemented redesign**.

Follow-up: [community research](community-research.md) looks at real feedback on
similar tools and pushes this proposal toward fewer permanent controls. In
particular, use one View selector instead of the multiple filter-chip rows
shown below.

## Recommendation

Finish the photographic model → configuration picker before adding more
dashboard features. Keep the bar popup quick to read and the companion app
focused on the task. Show less by default without cutting what the app can do.

This is a heuristic review of the QML and existing screenshots, plus inspection
of the installed game's code and assets. We didn't run a usability study or test
the proposed layout with users.

## Research translated into design decisions

| Finding | Application to Beamdeck |
| --- | --- |
| Progressive disclosure keeps rare options within reach without letting them compete with every task. [1] | Put frame timings, feed rate, charts and bridge troubleshooting under Details. Don't hide the garage the user asked for behind diagnostics. |
| People recognize things more easily than they recall names or identifiers. [2] | Show real model and configuration pictures with names, and support recent configurations and favorites. Never use identical synthetic cars in place of real previews. |
| Hierarchy comes from scale, contrast and grouping, not from putting a box around everything. [3] | Use one compact status row, one main content area and one action area. Use spacing instead of repeated bordered panels and all-caps headings. |
| Visible status and error prevention make actions predictable. [4] | Tell apart game stopped, starting, bridge unavailable, connected and disconnected. Show why an action is blocked next to the action. Keep the stationary and ownership checks and the confirmation step. |
| Small targets close together cause accidental clicks. [5] | Aim for 36-40 logical-pixel controls, visible keyboard focus and space between targets. W3C's 24 CSS-pixel web minimum is a reference, not a claim that native Qt meets that standard. |
| BeamNG uses UI layouts specific to each activity. [6] | Keep driving information in Drive and player information in Session. Don't render empty multiplayer widgets in single player. |

## Why the current UI feels cluttered

- The title, tagline, large status panel, tabs and permanent technical footer
  take up space before you start the task.
- Drive shows RPM in the gauge and again in a separate card, then adds six
  metrics, pedal bars, indicators, steering and G-force text, and a chart.
- Session gives trip statistics, performance diagnostics and charts the same
  emphasis.
- Garage shows model search, variant search, refresh, favorites, instructions
  and two competing lists all at once. At 500 pixels wide, a model list beside
  a variant pane is too narrow.
- Spawn and Replace on every card add visual noise and make accidental actions
  more likely.
- Permanent console commands and implementation explanations paper over missing
  states instead of helping with the current task.

### Keep, move, replace

| Keep readily visible | Move into secondary views | Replace |
| --- | --- | --- |
| Connection status, car name, speed, gear and RPM | Oil, coolant and boost, plus input telemetry, under Vehicle details | Large global status panel → compact row |
| Garage entry, photo cards and search | Charts, CPU/GPU timings and UI/feed rates under Diagnostics | Manual initial catalog load → automatic load on garage entry |
| Fuel and temperature warnings when relevant | Units, monitoring pause and troubleshooting under a menu | Two permanent searches → one search for the current browsing level |
| Multiplayer server and players when relevant | Trip history and acceleration estimates under Session details | Per-card action buttons → one selected-configuration action bar |

## Proposed information architecture

### Bar and popup

- The bar shows compact game state or speed, not a second dashboard.
- Keep the requested behavior where clicking a stopped game starts BeamNG, with
  a clear tooltip and feedback while it launches. Don't treat a missing
  telemetry feed as proof the game is stopped.
- Popup when the game is stopped: Start BeamNG, **Start BeamMP**, Open
  dashboard.
- Popup when connected: current vehicle, speed, fuel and multiplayer ping when
  available, plus Open dashboard and Garage shortcuts.
- Keep Start BeamMP available in the right running-game state. If activating the
  mod can't affect the current session, say that a restart is needed.
- No graphs, player table or technical counters in the popup.

### Companion app

Three primary destinations: Drive, Garage and Session. This grouping is a
proposal, not a reason to delete existing functionality.

- **Drive:** current vehicle, main gauge, fuel and contextual warnings. Vehicle
  details expand on demand.
- **Garage:** full-width model grid, then a full-width configuration grid. Keep
  browsing state when you return from a model.
- **Session:** trip summary, plus server and players when multiplayer is active.
  Performance diagnostics are secondary.
- Overflow menu: units, monitoring pause, diagnostics and keyboard help.

### Garage wireflow

```text
Garage                                      Refresh
[ Search models…                                ]
[ All ] [ Favorites ]

┌────────────────────┐  ┌────────────────────┐
│ Actual model photo │  │ Actual model photo │
│ ETK 800 Series     │  │ Civetta Scintilla  │
└────────────────────┘  └────────────────────┘

                      select a model
                             ↓

‹ Models / ETK 800 Series
[ Search configurations…                        ]
[ All ] [ Custom ] [ Favorites ]     Filters…

┌────────────────────┐  ┌────────────────────┐
│ Actual config photo│  │ Saved config photo │
│ Configuration name │  │ Configuration name │
│ RWD · Automatic    │  │ Custom             │
└────────────────────┘  └────────────────────┘

Selected configuration name
[ Spawn new ]                   [ Replace current ]
Contextual reason when unavailable
```

Layout starting points to check visually: 16-pixel page inset, 12-pixel grid
gaps, two columns around 500 pixels wide, one column when labels need the room,
and more columns in wider windows. Pictures keep their aspect ratio and don't
crop the vehicle. Use one vertical scroll surface for the grid, not nested
horizontal and vertical lists. Keep the selected action bar outside that scroll
area.

Selecting a card only selects it. It doesn't spawn anything. Escape or Back
returns to the model list with search and scroll position intact. Arrow keys
move, Enter selects, and labeled actions open a confirmation. Don't bind a bare
Enter key to destructive replacement.

## The missing photo/config feature: verified feasibility

The following comes from the installed game, not a guessed public API or an
internet screenshot:

`/mnt/shared/SteamLibrary/steamapps/common/BeamNG.drive/lua/ge/extensions/core/vehicles.lua`

| Data | Confirmed implementation |
| --- | --- |
| Model list | `core_vehicles.getModelList().models` (around line 1002) |
| Configurations for a model | `core_vehicles.getModel(modelKey).configs` (around lines 771-891) |
| Configuration identity | `model_key`, `key`, `pcFilename` (around lines 704-729) |
| User-facing names | `Configuration`, `Name` and `Description`; the game resolves localization keys |
| Configuration preview | `preview`, from a matching PNG or JPG (around line 736) |
| Model preview | `preview`, from the default PNG or JPG (around line 870) |
| Content origin | `Source`; custom PC files count as Custom unless an official or mod origin applies (around lines 550-568, 714-755) |
| Correct spawn options | `spawnNewVehicle(model, {config = config.pcFilename})`, with the same options for replace; keep the normal route that BeamMP overrides |

A read-only ZIP inspection confirmed `vehicles/etk800/default.jpg` (27,990
bytes), `844_150_A.jpg` (27,055 bytes), the matching `.pc` and configuration
metadata. That metadata includes drivetrain, transmission, power, torque and
weight, which can support optional filters and details. Don't assume saved
custom configurations have those specs.

The active user folder had saved `.pc` configurations with spaces and
parentheses. Some had matching JPGs and some didn't. So:

1. Don't restrict configuration keys to `[A-Za-z0-9_-]`, and don't split a
   filesystem path to work out identity. Keep model and configuration
   identifiers separate.
2. Check every action against the configuration allowlist the game provides at
   that moment. Reject traversal, arbitrary paths, stale selections and unknown
   identifiers, but don't reject legitimate display names.
3. Use the exact selected configuration. Never spawn the model default without
   saying so.
4. Exact config photo first, then a model photo labeled **Model preview**, then
   a clear **No preview available** state. A missing custom photo must not hide
   the saved configuration or pass as an accurate photo of its modifications.
5. Photos must be visible in Beamdeck, not only in the game's vehicle selector.

### Preview loading approach

- The game's `preview` value is a virtual filesystem path. It isn't always a
  file the desktop app can open.
- Resolve the real mounted origin through the game's filesystem metadata; the
  game itself uses `FS:getFileOriginInfo`. Check the archive-origin fields on a
  live install before you depend on their shape.
- Load loose user and unpacked-mod pictures directly after checking the
  canonical path. For ZIP content, read only the image member you've allowed,
  not a whole extracted vehicle archive. Use the asset that wins when mods
  override each other; don't guess that the model name equals the ZIP filename.
- Do image reads asynchronously through a Qt image provider [7]. Decode to the
  thumbnail size you need, cap input bytes and pixel dimensions, limit the
  number of workers and the cache size, and invalidate when the source changes.
- Use a virtualized Qt `GridView` [8]. Keep selection outside the delegates,
  and keep image loading independent of telemetry and gauge updates.
- Don't send image data in the 20 Hz telemetry stream, expose an image server
  without authentication, or redistribute proprietary game assets in the
  package or repository.

## Features worth including, in priority order

| Priority | Feature | Value and feasibility | UI cost |
| --- | --- | --- | --- |
| Required | Real model and config photos, plus saved custom configs | The core request we hadn't met; data and assets checked locally | Replaces the current text lists |
| Required | Reliable automatic hook and launch controls | An existing request; keep process presence separate from bridge health, activate the BeamMP mod, use the launcher's working directory and the supported `--no-launch` | A launch area that only appears when relevant |
| Next | Favorite exact configurations | Go back to a specific build instead of a model default; extend the existing model favorites | Star and Favorites filter |
| Next | Recent configurations | Faster repeat selection; at first, distinguish a requested change from a confirmed one | An optional Recent filter, not another dashboard panel |
| Next | Custom-only filter and clear source badges | Makes saved builds findable; uses the origin the game resolves, not guessed filenames | One filter chip and a small badge |
| Next | Remember search, scroll and selection | You don't have to restart the browsing task after looking at variants | No permanent controls |
| Next | Useful reconnect and error states | Explain a missing bridge versus a stopped game; retry without console instructions by default | Status only where it's relevant |
| Later | Drivetrain, transmission and source filters | Factory configs have the metadata; allow Unknown and don't make up specs for custom builds | A collapsible Filters control |
| Later | Switch among owned spawned vehicles | Existing active-vehicle and ownership data plus the normal switch API; recheck when the user confirms | A secondary owned-vehicles list |
| Later | Optional low-fuel and overheat alerts | Existing telemetry supports it; thresholds need tuning, hysteresis and a cooldown | Only appears when there's something to do |

Defer mod installation and updating, tuning editors, server browsing, live 3D
previews, arbitrary layout builders and more always-visible charts. They add a
lot of complexity before the requested workflow works. Never say a server
allows a spawn just because the local API accepted the request.

## Implementation blockers found in the current source

These are concrete defects, not reasons to keep changing timeouts:

- `src/VariantCard.qml` draws a synthetic car, tries to instantiate an object ID
  as a component, and references action state that isn't declared across
  components.
- `src/Main.qml` uses lists instead of the grid we asked for, shows both search
  levels at once, overwrites the model selection with a configuration ID, and
  has text that defers photos to the game.
- In `src/backend.cpp`, the general result handler consumes configuration
  results before the configuration-specific branch can run.
- `game/lua/ge/extensions/beamdeck.lua` calls `getModelsData()` without a
  qualifier, and the extension doesn't define it. It also passes a string where
  spawn and replace expect an options table. The game exports
  `core_vehicles.getModelsData`, but the right configuration interface here is
  `core_vehicles.getModel(modelKey).configs`.
- Recent UDP changes use unconnected-socket calls the wrong way and dropped
  session validation. Restore the fixed localhost peer and session design that
  was verified; C++ commands already come from the bound receiver socket. Don't
  blame ephemeral ports without evidence.
- Receiver results broadcast right now instead of going only to the requester.
  Restore the peer-port and request-ID checks, and cap configuration payload
  sizes.
- An invalid `../bad` model should be rejected. A missing forwarded datagram for
  that input is correct behavior, not proof of a socket timing bug. Put the
  rejection assertions back instead of accepting the input or relaxing
  freshness checks.

## Definition of done for the next implementation

- [ ] Real model photos render inside Beamdeck.
- [ ] Selecting a model opens a responsive configuration grid, with real
  per-config pictures where they exist.
- [ ] Saved custom names with spaces or parentheses show up and select the
  right `.pc`.
- [ ] Missing previews get a labeled fallback, and no synthetic replacement is
  shown as a photo.
- [ ] Browse and Back keep state, and fast model changes can't show stale
  variants or spawn the wrong selection.
- [ ] Spawn and Replace send the exact selected configuration, with
  confirmation, stationary and ownership checks, and normal server
  restrictions.
- [ ] Large config lists are chunked and size-capped instead of going over UDP
  limits or getting cut off without notice.
- [ ] Tests check real backend config contents, routing only to the requester,
  wrong-peer and unknown-ID rejection, invalid inputs and real QML interaction.
  A green busy flag alone doesn't test the config picker.
- [ ] Screenshots at narrow, default and wide sizes, keyboard navigation, theme
  behavior and longer translated labels are checked.
- [ ] Verify in read-only mode against the active game. Don't restart it or
  spawn or replace a vehicle just to test browsing.
- [ ] Package, install and live launch checks happen only after everything
  above. Don't call this research document an implementation.

## Sources

1. Nielsen Norman Group, [Progressive Disclosure](https://www.nngroup.com/articles/progressive-disclosure/).
2. Nielsen Norman Group, [Memory Recognition and Recall in User Interfaces](https://www.nngroup.com/articles/recognition-and-recall/).
3. Nielsen Norman Group, [Visual Hierarchy in UX](https://www.nngroup.com/articles/visual-hierarchy-ux-definition/).
4. Nielsen Norman Group, [10 Usability Heuristics](https://www.nngroup.com/articles/ten-usability-heuristics/), especially status, error prevention and minimalist design.
5. W3C, [Understanding Target Size (Minimum)](https://www.w3.org/WAI/WCAG22/Understanding/target-size-minimum.html).
6. BeamNG, [User Interface](https://documentation.beamng.com/getting_started/user_interface/).
7. Qt, [QQuickImageProvider](https://doc.qt.io/qt-6/qquickimageprovider.html), asynchronous loading and cache behavior.
8. Qt, [GridView](https://doc.qt.io/qt-6/qml-qtquick-gridview.html), delegate lifetime, keyboard navigation and layout.

Implementation evidence came from the installed `core/vehicles.lua`,
`content/vehicles/etk800.zip`, the active user-folder `vehicles/` and the
Beamdeck source at the time. External guidance informed the recommendation. The
proposed dimensions and feature priorities are design judgments to validate,
not research-proven constants.