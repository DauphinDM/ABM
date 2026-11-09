# ABM history: early UX research and redesign brief

**Archived pre-release design evidence.** Older names and proposals below are
historical, not the current ABM feature list. See [ABM](../../README.md).

Research date: 2026-11-09. Status: proposal and source audit, **not an implemented redesign**.

Follow-up: [community research](community-research.md) reviews actual feedback on similar tools and refines this proposal toward fewer permanent controls. In particular, use one View selector rather than the multiple filter-chip rows illustrated below.

## Recommendation

Finish the photographic model → configuration picker before adding more dashboard features. Keep the bar popup glanceable and the companion app task-focused. Reduce what is visible by default, not what the app can do.

This is a heuristic review of the QML and existing screenshots, plus inspection of the installed game's code/assets. It is not a usability study or a claim that the proposed layout has been tested with users.

## Research translated into design decisions

| Finding | Application to Beamdeck |
| --- | --- |
| Progressive disclosure keeps uncommon options available without making every task compete with them. [1] | Put frame timings, feed rate, charts and bridge troubleshooting under Details. Do not hide the requested garage behind diagnostics. |
| Recognition is easier than recalling names or identifiers. [2] | Show actual model/configuration pictures with names; support recent configurations and favorites. Never substitute identical synthetic cars for real previews. |
| Hierarchy comes from scale, contrast and grouping, not putting a box around everything. [3] | One compact status row, one main content area, one action area. Use spacing instead of repeated bordered panels and all-caps headings. |
| Visible status and error prevention make actions predictable. [4] | Distinguish game stopped, starting, bridge unavailable, connected and disconnected. Show why an action is blocked beside the action. Preserve stationary/ownership checks and confirmation. |
| Small adjacent targets cause accidental clicks. [5] | Aim for 36–40 logical-pixel controls, visible keyboard focus and separated targets. W3C's 24 CSS-pixel web minimum is a reference, not a claim of native Qt accessibility conformance. |
| BeamNG uses activity-specific UI layouts. [6] | Keep driving information in Drive and player information in Session; do not render empty multiplayer widgets during single player. |

## Why the current UI feels cluttered

- The title/tagline, large status panel, tabs and permanent technical footer consume space before the task starts.
- Drive repeats RPM in the gauge and a separate card, then adds six metrics, pedal bars, indicators, steering/G-force text and a chart.
- Session gives trip statistics, performance diagnostics and charts equal emphasis.
- Garage exposes model search, variant search, refresh, favorites, instructions and two competing lists simultaneously. At 500 pixels wide, a side-by-side model list and variant pane are too narrow.
- Repeating Spawn/Replace on every card adds visual noise and increases accidental-action risk.
- Permanent console commands and implementation explanations compensate for missing states rather than helping the current task.

### Keep, move, replace

| Keep readily visible | Move into secondary views | Replace |
| --- | --- | --- |
| Connection status, car name, speed/gear/RPM | Oil/coolant/boost and input telemetry under Vehicle details | Large global status panel → compact row |
| Garage entry, photo cards and search | Charts, CPU/GPU timings, UI/feed rates under Diagnostics | Manual initial catalog load → automatic load on garage entry |
| Contextual fuel/temperature warnings | Units, monitoring pause, troubleshooting under a menu | Two permanent searches → one search for the current browsing level |
| Multiplayer server and players when relevant | Trip history and acceleration estimates under Session details | Per-card action buttons → one selected-configuration action bar |

## Proposed information architecture

### Bar and popup

- Bar: compact game state or speed, not a second dashboard.
- Preserve the requested stopped-game click → Start BeamNG behavior, with clear tooltip and launch-in-progress feedback. Do not treat a missing telemetry feed as proof the game is stopped.
- Popup when stopped: Start BeamNG, **Start BeamMP**, Open dashboard.
- Popup when connected: current vehicle, speed, fuel, and multiplayer ping only when available; Open dashboard and Garage shortcuts.
- Start BeamMP remains available in the appropriate running-game state; report that a restart is required if activating the mod cannot affect the current session.
- No graphs, player table or technical counters in the popup.

### Companion app

Three primary destinations: **Drive · Garage · Session**. This grouping is a proposal, not a reason to delete existing functionality.

- **Drive:** current vehicle, main gauge, fuel, contextual warnings. Vehicle details expand on demand.
- **Garage:** full-width model grid, then full-width configuration grid. Preserve browsing state when returning from a model.
- **Session:** trip summary; server and players when multiplayer is active. Performance diagnostics are secondary.
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

Layout starting points, to validate visually: 16-pixel page inset, 12-pixel grid gaps, two columns around 500 pixels wide, one column when necessary for readable labels, more columns in wider windows. Pictures preserve aspect ratio and do not crop off the vehicle. Use a single vertical scroll surface for the grid, not nested horizontal/vertical lists. Keep the selected action bar outside it.

Selecting a card only selects it. It must not spawn anything. Escape/Back returns to models with search and scroll position preserved. Arrow keys navigate; Enter selects; explicit labeled actions open confirmation. Do not bind an unqualified Enter key to destructive replacement.

## The missing photo/config feature: verified feasibility

The following comes from the installed game, not an assumed public API or an internet screenshot:

`/mnt/shared/SteamLibrary/steamapps/common/BeamNG.drive/lua/ge/extensions/core/vehicles.lua`

| Data | Confirmed implementation |
| --- | --- |
| Model list | `core_vehicles.getModelList().models` (around line 1002) |
| Configurations for a model | `core_vehicles.getModel(modelKey).configs` (around lines 771–891) |
| Configuration identity | `model_key`, `key`, `pcFilename` (around lines 704–729) |
| User-facing names | `Configuration`, `Name`, `Description`; game resolves localization keys |
| Configuration preview | `preview`, using matching PNG/JPG (around line 736) |
| Model preview | `preview`, using default PNG/JPG (around line 870) |
| Content origin | `Source`; custom PC files are classified as Custom unless official/mod origin applies (around lines 550–568, 714–755) |
| Correct spawn options | `spawnNewVehicle(model, {config = config.pcFilename})`, with equivalent options for replace; retain the normal BeamMP-overridden route |

Read-only ZIP inspection confirmed `vehicles/etk800/default.jpg` (27,990 bytes), `844_150_A.jpg` (27,055 bytes), the matching `.pc`, and configuration metadata. Metadata includes drivetrain, transmission, power, torque and weight; these can support optional filters/details. Do not assume saved custom configurations contain those specifications.

The active user folder contains saved `.pc` configurations with spaces and parentheses; some have matching JPGs and some do not. Therefore:

1. Do not restrict configuration keys to `[A-Za-z0-9_-]` or split a filesystem path to infer identity. Carry model and configuration identifiers separately.
2. Resolve every action against the current game-provided configuration allowlist. Reject traversal, arbitrary paths, stale selections and unknown identifiers without rejecting legitimate display names.
3. Use the exact selected configuration, never silently spawn the model default.
4. Exact config photo → model photo labeled **Model preview** → explicit **No preview available**. A missing custom photo must not hide the saved configuration or be presented as an accurate photo of its modifications.
5. Photos must be visible in Beamdeck, not only in the game's vehicle selector.

### Preview loading approach

- The game's `preview` is a virtual filesystem path, not necessarily a file the desktop app can open.
- Resolve the actual mounted origin using the game's filesystem metadata (`FS:getFileOriginInfo` is used by the game itself). Verify archive-origin fields live before depending on their shape.
- Load loose user/unpacked-mod pictures directly after canonical-path validation. For ZIP content, read only the allowlisted image member, not an entire extracted vehicle archive. Respect the mounted winner when mods override assets; do not guess that model name equals ZIP filename.
- Implement asynchronous, bounded image reads through a Qt image provider [7]; decode to the required thumbnail size, cap input bytes/pixel dimensions, limit worker concurrency and cache size, and invalidate on source changes.
- Use a virtualized Qt `GridView` [8]. Keep selection outside delegates and image loading independent of telemetry/gauge updates.
- Do not send image data in the 20 Hz telemetry stream, expose an unauthenticated image server, or redistribute proprietary game assets in the package/repository.

## Features worth including, in priority order

| Priority | Feature | Value and feasibility | UI cost |
| --- | --- | --- | --- |
| Required | Actual model/config photos and saved custom configs | Core unfulfilled request; data/assets verified locally | Replaces the current text lists |
| Required | Reliable automatic hook and launch controls | Existing request; separate process presence from bridge health; activate BeamMP mod, use launcher working directory and supported `--no-launch` | State-dependent launch area only |
| Next | Favorite exact configurations | Return to a specific build rather than a model default; extend existing model favorites | Star + Favorites filter |
| Next | Recent configurations | Faster repeat selection; initially distinguish requested from confirmed vehicle changes | Optional Recent filter, not another dashboard panel |
| Next | Custom-only filter and clear source badges | Makes saved builds findable; based on game-resolved origin, not guessed filenames | One filter chip + small badge |
| Next | Remember search/scroll/selection | Avoid restarting the browsing task after inspecting variants | No permanent controls |
| Next | Useful reconnect/error states | Explain missing bridge versus stopped game; retry without console instructions by default | Contextual status only |
| Later | Drivetrain/transmission/source filters | Metadata exists for factory configs; allow Unknown and avoid fabricated custom specs | A collapsible Filters control |
| Later | Switch among owned spawned vehicles | Existing active-vehicle/ownership data and normal switch API; revalidate at confirmation | A secondary owned-vehicles list |
| Later | Optional low-fuel/overheat alerts | Existing telemetry supports it; thresholds need tuning, hysteresis and cooldown | Only appears when actionable |

Defer mod installation/updating, tuning editors, server browsing, live 3D previews, arbitrary layout builders and more always-visible charts. They introduce substantial complexity before the requested workflow works. Never claim a server permits a spawn merely because the local API accepted a request.

## Implementation blockers found in the current source

These are concrete defects, not reasons to keep changing timeouts:

- `src/VariantCard.qml` draws a synthetic car, attempts to instantiate an object ID as a component, and references undeclared cross-component action state.
- `src/Main.qml` uses lists instead of the requested grid, displays both search levels together, overwrites model selection with a configuration ID, and contains text explicitly deferring photos to the game.
- `src/backend.cpp`: the general result handler consumes configuration results before the configuration-specific branch can run.
- `game/lua/ge/extensions/beamdeck.lua`: calls an undefined, unqualified `getModelsData()` from the extension and passes a string where spawn/replace expects an options table. The game exports `core_vehicles.getModelsData`, but the appropriate configuration interface here is `core_vehicles.getModel(modelKey).configs`.
- Recent UDP changes use unconnected-socket calls incorrectly and lost session validation. Restore the verified fixed localhost peer/session design; C++ commands already originate from the bound receiver socket. Do not blame ephemeral ports without evidence.
- Receiver results currently broadcast instead of routing only to the requester; restore peer-port/request-ID checks and bound configuration payloads.
- An invalid `../bad` model is supposed to be rejected. A missing forwarded datagram for that input is correct behavior, not evidence of a socket timing bug. Reinstate rejection assertions rather than accepting it or relaxing freshness checks.

## Definition of done for the next implementation

- [ ] Real model photos render inside Beamdeck.
- [ ] Selecting a model opens a responsive configuration **grid**, with real per-config pictures where present.
- [ ] Saved custom names with spaces/parentheses appear and select the correct `.pc`.
- [ ] Missing previews have honest, labeled fallback behavior; no synthetic replacements passed off as photos.
- [ ] Browse/Back preserves state; fast model changes cannot show stale variants or spawn the wrong selection.
- [ ] Spawn/Replace sends the exact selected configuration with confirmation, stationary and ownership checks, and normal server restrictions.
- [ ] Large config lists are chunked/bounded rather than silently exceeding UDP limits or truncating without notice.
- [ ] Tests exercise actual backend config contents, requester-only routing, wrong-peer/unknown-ID rejection, invalid inputs and real QML interaction. A green busy flag alone is not a config-picker test.
- [ ] Screenshots at narrow/default/wide sizes, keyboard navigation, theme behavior and longer translated labels are checked.
- [ ] Verify read-only against the active game; do not restart it or spawn/replace a vehicle just to test browsing.
- [ ] Package/install and live launch checks happen only after the above; do not call this research document an implementation.

## Sources

1. Nielsen Norman Group, [Progressive Disclosure](https://www.nngroup.com/articles/progressive-disclosure/).
2. Nielsen Norman Group, [Memory Recognition and Recall in User Interfaces](https://www.nngroup.com/articles/recognition-and-recall/).
3. Nielsen Norman Group, [Visual Hierarchy in UX](https://www.nngroup.com/articles/visual-hierarchy-ux-definition/).
4. Nielsen Norman Group, [10 Usability Heuristics](https://www.nngroup.com/articles/ten-usability-heuristics/), especially status, error prevention and minimalist design.
5. W3C, [Understanding Target Size (Minimum)](https://www.w3.org/WAI/WCAG22/Understanding/target-size-minimum.html).
6. BeamNG, [User Interface](https://documentation.beamng.com/getting_started/user_interface/).
7. Qt, [QQuickImageProvider](https://doc.qt.io/qt-6/qquickimageprovider.html), asynchronous loading and cache behavior.
8. Qt, [GridView](https://doc.qt.io/qt-6/qml-qtquick-gridview.html), delegate lifetime, keyboard navigation and layout.

Implementation evidence: installed `core/vehicles.lua`, `content/vehicles/etk800.zip`, active user-folder `vehicles/` and the current Beamdeck source. External guidance informs the recommendation; the proposed dimensions and feature priorities are design judgments to validate, not research-proven constants.
