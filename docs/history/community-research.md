# Community research: similar BeamNG tools and minimal menus

**ABM historical research.** Product names below come from the pre-release design
phase, not current branding. See the [current ABM guide](../../README.md).

Research date: 2026-11-09. This is a qualitative review, not a representative
survey, an implementation or a popularity ranking.

## Scope and evidence quality

We searched the web for standalone vehicle selectors, companion apps,
second-monitor dashboards, custom configurations and UI complaints. Full
discussion text was readable on the BeamNG forums and resource reviews. Reddit
pages didn't return readable content through the fetch tool, so we used search
snippets only as leads, not as verified comment threads. We didn't download or
run any third-party tools.

One precedent is especially close: **Ellexium's Advanced Vehicle Selector**, an
external app that runs beside BeamNG and shows model and configuration
previews, search and favorites. Its release thread has a lot of practical
feedback. Enhanced Dashboard gives us a second, independent source on
information density and discoverability. Draco's Tuner adds feedback about
second screens and minimizing the app.

Several detailed selector suggestions came from one contributor. Repeated
replies or quotes from that person don't count as independent votes. Below, we
separate author feature lists from actual user requests.

## What people actually said

| Evidence | Type | Implication for Beamdeck |
| --- | --- | --- |
| A selector user asked for search to clear when emptied, a clear button or Escape, larger controls, direct navigation through long config lists and a switcher that stays visible. [S2, post 37] | One user's direct requests | Use one search with a clear button, predictable Escape behavior and comfortable targets. Avoid navigation that only goes next and previous. An optional pin is worth considering later. |
| One user had over 100 Sunburst configurations but saw only 34. [S2, post 28] Others reported missing mod detection and installation problems. [S4, posts 64-66] | User reports | Complete configurations and automatic discovery matter more than extra styling. Show counts, and make it clear when results are filtered versus missing. |
| A user couldn't work out how to add favorites. [S4, post 78] | One clear discoverability problem | Keep the favorite star visible, with an accessible label and tooltip. Don't hide the only way to favorite behind right-click or double-click. |
| A user wanted custom vehicle groups, such as separate Ford and Gavril groups. [S4, post 79] | One direct feature request | Consider optional collections later. Don't add a group editor or permanent category sidebar right away. |
| Multiple users reported console-pasting and focus problems. The author later removed console automation. [S1, posts 2-5; S2, posts 21, 26] | Repeated reports plus author response | Use the bridge and normal game APIs, not simulated keyboard input. Browsing shouldn't steal focus. |
| A second-monitor user reported the game minimizing when the selector was focused in fullscreen. [S3, posts 56-57] | One report plus author explanation | Test the Linux/Hyprland interaction, and don't assume Windows behavior applies the same way. Offer a short borderless-window hint only if it's relevant. |
| Users reported wrong categorization, long cache initialization and configs that didn't match the in-game selector. [S3, posts 58-60; S4, post 70] | Reports, partly from one contributor | Use the catalog identity and asset origin the game resolves, keep model and config relationships intact, and show real loading status. |
| A later report says a successful spawn was retried seven times because completion wasn't recognized. [S4, post 80] | One serious correctness report | Never retry a spawn request automatically after an ambiguous timeout; spawning isn't idempotent. Match request IDs and report the uncertainty instead of duplicating cars. |
| A dashboard user couldn't find navigation because it required clicking the speedometer. The author acknowledged that discoverability needed work. [D1, posts 4-6] | User problem plus author acknowledgement | Minimalism can't mean invisible controls. Keep Garage and the other primary destinations labeled. |
| A dashboard user wanted parking assistance without the visible UI; another wanted to turn all assistance off. [D1, posts 8-10] | Two different users' requests | Let the window disappear while the receiver keeps running. Don't bundle automatic driving assistance into a telemetry companion by default. |
| One dashboard user considered asking for axis indicators but decided they would clutter the clean design. [D1, post 14] | A clear preference from one user | Put pedal, G-force and steering displays in optional details, not the default Drive view. |
| A dashboard user disliked automatic switches to pitch/roll gauges during truck gear changes. [D1, post 20] | One direct complaint | Context can change what information is available, but it must not replace the page the user picked. |
| A dashboard user requested Fahrenheit support. [D1, post 4] | One direct feature request | Put units in one simple preference instead of adding permanent unit controls beside every metric. |
| A tuner reviewer requested a minimize button; another couldn't find the app menu. [T1, July 27 and September 6 reviews] | Two user reports | Make open, close and minimize easy to find, and label the navigation. Hidden shortcuts alone aren't enough. |

## What similar tools already offer (not proof of demand by itself)

- **Advanced Vehicle Selector:** real pictures, favorites for exact
  configurations, model-scoped search, optional pinned favorites, source and
  ZIP filtering, and a hide-from-list option you can undo. The author says slow
  switching to the built-in selector was the motivation. [S1]
- **Enhanced Dashboard:** compact, analog and digital choices, separate trip
  widgets, optional information and settings. It's a useful precedent for
  separating concerns, but its long assists and navigation feature list doesn't
  mean we should copy everything. [D1]
- **Draco's Tuner:** a separate-screen and browser workflow with named saved
  tunes. Its tuning scope is quite different from Beamdeck, and that's not a
  reason to add ECU editing. [T1]

## What this changes in the Beamdeck design

The evidence points to a fast, complete, predictable selector and an
unobtrusive dashboard, not an all-in-one mod management suite.

### Permanent navigation

Only Drive, Garage and Session, plus one labeled Settings menu. You shouldn't
need to click a gauge, hover an invisible area or open a context menu to reach a
primary destination.

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

- Use one search field for the current level. Clearing it brings the results
  back right away. Escape clears a focused search that isn't empty. Otherwise
  it goes back or closes the top overlay. Always show a Back button.
- A single View selector offers All, Custom and Favorites. Recent can come
  after exact configuration tracking works. This replaces several permanent
  filter-chip rows.
- Optional Filters, Refresh and Show hidden live in the garage overflow menu.
  Show an active-filter indicator and a clear reset while a filter is on.
  Controls you can't see must not make filtered content look missing.
- Use full-width virtualized grids. Don't squeeze a model sidebar next to a
  second grid in the narrow window. Keep search, the selected item and scroll
  position when you go Back.
- Real model and configuration photos are core content. A missing photo
  doesn't remove a saved config. Label fallback model pictures as fallbacks.
- Keep favorite stars visible. Extra specs appear only for the selected
  configuration or in Details, not on every card.
- Use one shared action bar, not two action buttons on every card. Make
  selection and confirmation clear, and don't replace a vehicle on an
  unexpected double-click.
- Keep the selected page steady through gear changes and telemetry updates.

### Popup and Drive

- Popup: game state, current car and a few useful values, plus Dashboard and
  Garage access. Start BeamNG and Start BeamMP appear when they're relevant,
  not beside every connected-state metric.
- Drive: one main gauge, fuel and warnings you can act on. Vehicle details
  expand when you ask. No permanent charts, raw feed rates or CPU/GPU
  frame-time cards.
- Closing the dashboard doesn't stop telemetry or disable the plugin. Optional
  pinning is a later setting, not a permanent toolbar button.

### Settings budget

Start with four entries people can understand: Units, Interface size,
Monitoring, Help & diagnostics. Reuse platform text and display scaling where
you can. Don't expose cache paths, port numbers, thumbnail workers or
configuration limits as normal preferences.

## Feature priorities after research

1. **Required first:** real photos, complete discovery of factory, mod and
   custom configurations, and spawning the exact selected config. The user
   already required this; it isn't waiting on community validation.
2. **High value, small UI footprint:** visible config favorites, clearable
   search for the current level, remembered browsing state, readable controls,
   and clear connection, loading and error states.
3. **Optional later:** Recent, a reversible Hide from list, custom collections
   and optional pinning. Collections and pinning are individual requests in
   this sample, not proven broad demand.
4. **Defer:** traffic queues, mod deletion and installation, color and tuning
   editors, browser and phone streaming, media players, navigation and
   automatic driving assists. They add dependencies, safety and security
   concerns, and menu complexity beyond the current job.

Don't copy the precedent's option to hide every config without a picture by
default. That would directly undercut the user's saved-config requirement.
Don't copy retry-on-timeout spawning or console automation either.

## Research limitations

This sample points to concrete needs and failure modes; it doesn't show which
feature is most popular. Most selector feedback is from 2025, and the dashboard
and review examples include 2026. They cover different game versions, Windows
workflows, and in-game versus standalone UIs. We don't assume any of it maps
directly to Omarchy or Hyprland. The searches didn't find an exact Omarchy
bar-plugin equivalent. We didn't use Reddit snippets to invent comments, dates,
votes or consensus.

## Sources

- **S1:** [Advanced Vehicle Selector, introduction and discussion](https://www.beamng.com/threads/advanced-vehicle-selector.103341/), March 2025 onward.
- **S2:** [Selector discussion, page 2](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-2), especially [post 37](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-2#post-1821859).
- **S3:** [Selector discussion, page 3](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-3), including [fullscreen report](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-3#post-1826573).
- **S4:** [Selector discussion, page 4](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4), including [favorites discoverability](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4#post-1864759), [custom groups](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4#post-1865499) and [duplicate-spawn report](https://www.beamng.com/threads/advanced-vehicle-selector.103341/page-4#post-1889593).
- **D1:** [Enhanced Dashboard with Realistic Driving Features](https://www.beamng.com/threads/enhanced-dashboard-with-realistic-driving-features.111781/), September-October 2026 feedback; especially [clutter comment](https://www.beamng.com/threads/enhanced-dashboard-with-realistic-driving-features.111781/#post-1970371).
- **T1:** [Draco's Tuner overview and user reviews](https://www.beamng.com/resources/dracos-tuner.38692/), July-September 2026 feedback.

See [the UX/technical brief](ux-research.md) for local image and config API
evidence and acceptance criteria. This community review refines the brief's
menu proposal: one View selector replaces the earlier rows of filter chips.