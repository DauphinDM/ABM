# Community research: similar BeamNG tools and minimal menus

**ABM historical research.** Product names below reflect the pre-release design
phase, not current branding. See the [current ABM guide](../../README.md).

Researched 2026-11-09. This is a qualitative review, not a representative survey, implementation, or popularity ranking.

## Scope and evidence quality

Web searches covered standalone vehicle selectors, companion apps, second-monitor dashboards, custom configurations and UI complaints. Full discussion text was readable on the BeamNG forums and resource reviews. Reddit pages did not return readable content through the fetch tool, so search snippets were used only as leads, not as verified comment threads. No downloads or execution of third-party tools were needed.

There is a particularly close precedent: **Ellexium's Advanced Vehicle Selector**, an external application that runs beside BeamNG and displays model/configuration previews, search and favorites. Its release thread has substantial practical feedback. Enhanced Dashboard provides a second, independent source about information density and discoverability. Draco's Tuner provides additional second-screen and minimize/discoverability feedback.

Several detailed selector suggestions came from the same contributor. Repeated replies or quotes of that contributor are not independent votes. Author feature lists are distinguished below from actual user requests.

## What people actually said

| Evidence | Type | Implication for Beamdeck |
| --- | --- | --- |
| A selector user requested clearing search when emptied, a clear button or Escape; larger UI controls; direct navigation through long config lists; and a reliably visible switcher. [S2, post 37] | One user's explicit requests | One search with a clear button; predictable Escape behavior; comfortable targets. Avoid next/previous-only navigation. An optional pin is worth considering later. |
| A user reported having over 100 Sunburst configurations but seeing only 34. [S2, post 28] Other users reported missing mod detection and installation problems. [S4, posts 64–66] | User reports | Configuration completeness and automatic discovery matter more than extra styling. Show counts and distinguish filtered results from missing content. |
| A user could not work out how to add favorites. [S4, post 78] | One explicit discoverability problem | Keep the favorite star visible with an accessible label/tooltip. Do not put the only way to favorite behind right-click or double-click. |
| A user wanted custom vehicle groups, such as separate Ford and Gavril groups. [S4, post 79] | One explicit feature request | Consider optional collections later. Do not immediately add a group editor or a permanent category sidebar. |
| Multiple users reported console-pasting/focus problems; the author subsequently removed console automation. [S1, posts 2–5; S2, posts 21, 26] | Repeated reports plus author response | Use the bridge and normal game APIs, not simulated keyboard input. Do not steal focus as part of browsing. |
| A second-monitor user reported the game minimizing when the selector was focused in fullscreen. [S3, posts 56–57] | One report plus author explanation | Test the Linux/Hyprland interaction; do not assume the Windows behavior applies identically. Offer a concise borderless-window hint only if relevant. |
| Users reported wrong categorization, long cache initialization and configs that did not match the in-game selector. [S3, posts 58–60; S4, post 70] | Reports, partly from one contributor | Use game-resolved catalog identity and asset origin, preserve model/config relationships and make loading status honest. |
| A later report says a successful spawn was retried seven times because completion was not recognized. [S4, post 80] | One serious correctness report | Never automatically retry non-idempotent spawn requests after an ambiguous timeout. Correlate request IDs and report uncertainty instead of duplicating cars. |
| A dashboard user could not find navigation because it required clicking the speedometer; the author acknowledged discoverability needed improvement. [D1, posts 4–6] | User problem plus author acknowledgement | Minimalism must not mean invisible controls. Keep Garage and other primary destinations explicitly labeled. |
| A dashboard user wanted parking assistance without the visible UI; another wanted to turn all assistance off. [D1, posts 8–10] | Two different users' requests | Allow the window to disappear while the receiver continues. Do not bundle automatic driving assistance into a telemetry companion by default. |
| One dashboard user considered asking for axis indicators but decided they would clutter the clean design. [D1, post 14] | Explicit preference from one user | Pedal/G-force/steering displays belong in optional details, not the default Drive view. |
| A dashboard user disliked automatic switches to pitch/roll gauges during truck gear changes. [D1, post 20] | One explicit complaint | Context can change available information, but must not automatically replace the page the user selected. |
| A dashboard user requested Fahrenheit support. [D1, post 4] | One explicit feature request | Keep units under a simple preference rather than adding separate permanent unit controls beside each metric. |
| A tuner reviewer requested a minimize button; another could not find the app menu. [T1, July 27 and September 6 reviews] | Two user reports | Obvious open/close/minimize access and labeled navigation are more valuable than hidden shortcuts alone. |

## What similar tools already offer—not proof of demand by itself

- **Advanced Vehicle Selector:** actual pictures, favorites for exact configurations, model-scoped search, optional pinned favorites, source/ZIP filtering and reversible hide-from-list behavior. The author identifies slow switching to the built-in selector as the motivation. [S1]
- **Enhanced Dashboard:** compact/analog/digital choices, separate trip widgets, optional information and settings. Useful precedent for separating concerns, but its extensive assists/navigation feature list is not a mandate to copy everything. [D1]
- **Draco's Tuner:** a separate-screen/browser workflow and named saved tunes. Its tuning scope is substantially different from Beamdeck and is not a reason to add ECU editing. [T1]

## What this changes in the Beamdeck design

The evidence supports a **fast, complete, predictable selector** and an **unobtrusive dashboard**, not an all-in-one mod management suite.

### Permanent navigation

Only **Drive · Garage · Session**, plus one labeled Settings menu. Primary destinations must not require clicking a gauge, hovering an invisible region or opening a context menu.

### Garage: one task at a time

```text
Garage                                         ⋯
[ Search models…                         × ]
View: All ▾

             Model photo grid

                   select
                      ↓

‹ Models / Selected model                      ⋯
[ Search configurations…                 × ]
View: All ▾                         24 configs

          Configuration photo grid
          photo · name · favorite star

Selected configuration
[ Spawn new ]             [ Replace current ]
```

- One search field for the active level. Clearing it restores results immediately. Escape clears a focused nonempty search; otherwise it goes back/dismisses the topmost overlay. A visible Back button is always available.
- A single **View** selector offers All, Custom and Favorites. Recent may be added after exact configuration tracking works. This replaces multiple permanent filter-chip rows.
- Optional **Filters…**, **Refresh** and **Show hidden** live in the garage overflow, with a visible active-filter indicator and a clear reset when filtering is in effect. Hidden controls must not make filtered content appear missing.
- Full-width virtualized grids; no cramped model sidebar next to a second grid in the narrow window. Preserve search, selected item and scroll position on Back.
- Real model/configuration photos are core content. Missing photos do not remove saved configs. Fallback model pictures are labeled as such.
- Favorite stars remain visible. Additional specs appear only for the selected configuration or on explicit Details, not on every card.
- One shared action bar, not two action buttons on every card. Explicit selection/confirmation; no surprise double-click replacement.
- Keep the selected page stable across gear changes and telemetry updates.

### Popup and Drive

- Popup: game state, current car and a few useful values; Dashboard/Garage access. Start BeamNG/Start BeamMP appear when relevant, not beside every connected-state metric.
- Drive: one main gauge, fuel and actionable warnings. Vehicle details expand on request. No permanent charts, raw feed rates or CPU/GPU frame-time cards.
- Closing the dashboard does not stop telemetry or disable the plugin. Optional pinning is a later setting, not a permanent toolbar button.

### Settings budget

Start with four understandable entries: **Units**, **Interface size**, **Monitoring**, **Help & diagnostics**. Reuse platform text/display scaling where possible. Do not expose cache paths, port numbers, thumbnail workers or configuration limits as normal preferences.

## Feature priorities after research

1. **Required first:** actual photos, complete factory/mod/custom configuration discovery and exact selected-config spawning. This is already the user's requirement, not something awaiting community validation.
2. **High value, small UI footprint:** visible config favorites; clearable scoped search; remembered browsing state; readable controls; clear connection/loading/error states.
3. **Optional later:** Recent, reversible Hide from list, custom collections, optional pinning. Collections and pinning are individual requests in this sample—not established broad demand.
4. **Defer:** traffic queues, mod deletion/installation, color/tuning editors, browser/phone streaming, media players, navigation and automatic driving assists. These add dependencies, safety/security concerns and menu complexity beyond the current job.

Do not copy the precedent's option to hide all configs without pictures as a default: it would directly undermine the user's saved-config requirement. Do not copy retry-on-timeout spawning or console automation.

## Research limitations

This sample identifies concrete needs and failure modes, not which feature is most popular. Most selector feedback dates from 2025; dashboard/review examples include 2026. They span different game versions, Windows workflows and in-game versus standalone UIs. No equivalence to Omarchy/Hyprland behavior is assumed. Searches did not establish an exact Omarchy bar-plugin equivalent. Reddit snippets were not used to invent comments, dates, votes or consensus.

## Sources

- **S1:** [Advanced Vehicle Selector, introduction and discussion](https://www.beamng.com/threads/advanced-vehicle-selector.103341/), March 2025 onward.
- **S2:** [Selector discussion, page 2](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-2), especially [post 37](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-2#post-1821859).
- **S3:** [Selector discussion, page 3](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-3), including [fullscreen report](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-3#post-1826573).
- **S4:** [Selector discussion, page 4](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4), including [favorites discoverability](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4#post-1864759), [custom groups](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4#post-1865499) and [duplicate-spawn report](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4#post-1889593).
- **D1:** [Enhanced Dashboard with Realistic Driving Features](https://www.beamng.com/threads/enhanced-dashboard-with-realistic-driving-features.111781/), September–October 2026 feedback; especially [clutter comment](https://www.beamng.com/threads/enhanced-dashboard-with-realistic-driving-features.111781/#post-1970371).
- **T1:** [Draco's Tuner overview and user reviews](https://www.beamng.com/resources/dracos-tuner.38692/), July–September 2026 feedback.

See [the UX/technical brief](ux-research.md) for local image/config API evidence and acceptance criteria. This community review refines that brief's menu proposal: a single View selector replaces the earlier multiple filter-chip rows.
