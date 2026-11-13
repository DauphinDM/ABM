# Next feature requests

Status: **Planning only. Not implemented.**

## 1. Save god-mode vehicle configurations

- Look into saving ABM god-mode/protection and tuning settings into a native BeamNG vehicle configuration file.
- Check which settings BeamNG actually restores. Don't assume runtime Lua changes can be written into a normal `.pc` file.
- If native configuration storage can't keep these settings, give ABM presets saved per vehicle and, where possible, per configuration.
- Keep saved presets separate from temporary ordinary-reset recipes. Define how you save, load, update and delete them.
- Preserve existing custom configurations and keep the ownership, capability and server-policy checks when loading settings. Saving a preset must not bypass restrictions.

## 2. Correct previews for custom and modded vehicles

- Prefer the actual image that belongs to the selected configuration, including custom configurations and configurations for modded vehicles.
- If there's no configuration image, use the right vehicle/model preview when one is available, and label it clearly as a model preview.
- Look into using BeamNG's own default placeholder when neither the configuration nor the model has a usable image.
- Handle missing, corrupt and unreadable images, plus custom/modded vehicles with no default car image. Never swap in a photo of an unrelated vehicle.
- Avoid bundling large artwork or duplicating game assets. A small local fallback is fine only if the game's placeholder can't be used reliably. Custom artwork isn't a priority.
- Reuse locally installed previews where you can. Any future generated-preview cache needs size limits and a clear cleanup plan.

No application changes, asset generation or implementation are authorized by this note alone.

## 3. Research findings

Research date: **2026-11-10**. Baseline: ABM 0.10 development. This section extends the two requests above and complements [the larger research roadmap](massive-update-research.md). Some older roadmap items may be implemented by now. Everything below is still planning, not an announcement of working features.

Evidence labels:

- **Documented:** the linked BeamNG documentation supports it directly.
- **Community lead:** a reported workflow, not a supported API or behavior we tested ourselves.
- **Proposal:** ABM design reasoning. Implementation and compatibility still need validation.

### 3.1 Native configuration versus an ABM preset

**Documented [S1, S2]:** native configurations represent parts, tuning variables, model, paint information and license-plate settings. The tuning controls you get depend on the installed parts and the vehicle. Saving a configuration can also generate a screenshot thumbnail. The documentation recommends not overwriting vanilla configurations.

**Not established:** whether arbitrary runtime torque-curve edits, overridden damage functions, resized physical nodes or expanded gearbox/controller tables survive a native `.pc` save and load. The documentation doesn't define those ABM adapters as native configuration fields. That isn't proof every custom field gets rejected. It means ABM must not promise a portable god-mode `.pc` without a real save and reload test.

**Recommended proposal: a paired save.**

1. Save supported parts, paint and native tuning through the game's configuration mechanism, under a new name you choose.
2. Store ABM-only accepted settings in a versioned JSON sidecar tied to that model and exact configuration. A proposed filename convention is `<config>.abm.json`; this isn't an existing BeamNG format.
3. Show two clear actions: **Load normal configuration** and **Load configuration + ABM preset**. Explain that the second one needs ABM and a compatible bridge. Don't imply god mode travels inside a plain `.pc` file.
4. On load, wait for fresh vehicle readback after any respawn, check the selected model/configuration and supported adapters, then validate server permission before applying. Don't replay old vehicle IDs or generation tokens from disk.
5. If native saving can't be integrated safely, ship a standalone ABM per-car preset first. Tie it to a model/configuration reference without touching the original configuration.

Suggested sidecar contents: schema version, preset name, model identifier, configuration reference, relevant parts/configuration fingerprint, ABM/adapter version, required capabilities and accepted vehicle settings. Store only finite numeric/boolean data: no Lua, shell commands, credentials or global server-policy overrides. World gravity and one-shot actions shouldn't go into a car preset by default.

Important edge cases:

- A display name alone isn't a stable identity. Duplicate names, renamed configs, replaced mod files and missing parts need a visible mismatch state.
- Native settings and ABM adapters may overlap. Define ownership and application order so a runtime gearbox or power override doesn't quietly undo native tuning values.
- A paired `.pc` and sidecar save isn't one atomic operation. Report partial failure. Don't claim both were saved when only one write succeeded. Keep previous files intact and use atomic replacement for files ABM owns.
- Loading must show skipped/unsupported settings and partial failures, not silently substitute stock values. An optional dry-run compatibility report should come before risky application.
- Persistent disk presets are different from the existing in-memory ordinary-reset recipe. Reset persistence doesn't replace Save/Load.

### 3.2 Accurate previews without shipping an image library

**Documented [S1, S2]:** BeamNG saves thumbnails beside configurations and has a Showroom thumbnail-generation workflow. The modding guide warns that generated images can be very large and recommends resizing and JPEG conversion. Local previews are therefore the best first source, not downloaded or newly bundled art.

**Proposed resolution order:**

1. Exact configuration image supplied by the game's catalog, including images inside a mod archive.
2. Exact configuration image in the active user folder, when its association is verified.
3. Correct model preview, visibly labeled **Model preview** instead of being passed off as the exact custom car.
4. BeamNG's default missing-preview asset, if its installed path, supported format and availability can be verified.
5. Existing compact ABM missing-preview treatment if the game asset is unavailable. No unrelated car image, random web image or large custom art pack.

The precedence between catalog and user-folder images must follow the game's actual mounted-file precedence, not a guess that an older archive wins. Validate decoding before accepting a candidate. Missing, corrupt and unreadable images must all reach the fallback path.

**Unresolved:** this research hasn't verified the exact placeholder asset or a stable single-configuration capture API. Don't invent a path or promise a universal API.

**Community lead [S5]:** a 2024 forum discussion describes generating a single thumbnail while saving one configuration and reports that the general generator can process all configurations. Treat it as a lead for current-version investigation. Don't copy advice to move other configs out of the game or overwrite existing saves just to get an image.

An optional **Capture preview for this config** should be a later action you choose yourself. Never change map, spawn/reset a car, move its camera or batch-render mods just because the Garage opened. If capture is supported, restore temporary camera/UI state where possible and tell the user about any state you can't restore.

Proposed cache controls: on-demand decoding, limits on image dimensions, pixel count and archive reads, thumbnail-sized derivatives, an overall byte budget, deduplication, source-change invalidation and **Clear preview cache**. Cache keys should include model/config identity and source revision. Cache cleanup must not delete original game/mod/user images. Proposed dimensions and budgets need measurement before they become defaults.

## 4. Additional features worth adding

These extend existing controls instead of relabeling live power, RPM, clutch protection, reset recipes or Garage filters as new work. Effort is relative, not a time estimate. Source references establish game concepts, not ready-made ABM APIs.

| Priority | Proposal | Why it is useful | Evidence and limits | Effort / risk |
| --- | --- | --- | --- | --- |
| P0 | Named per-config preset library | Save god-mode, drift, towing or experiment setups between sessions. Rename, duplicate and load them when you choose. | Proposal based on native config structure [S1, S2]; ABM-only data needs its own adapter contract. | Medium / medium |
| P0 | Preset compatibility and dependency report | Identify changed parts, missing configuration/mod references and unsupported adapters before loading a tune. | Parts are dependent and tuning availability changes with installed parts [S2]. Exact mod-version resolution isn't verified yet. | Medium / low for read-only reporting |
| P0 | Preview provenance and repair panel | Explain “exact config,” “model fallback,” “missing,” “corrupt” or “stale”; refresh only the affected preview. | Config thumbnails are documented [S1, S2]. Default-placeholder lookup and invalidation need verification. | Small to medium / low if read-only |
| P1 | Preset comparison with selective loading | Compare current accepted values to a saved setup. Load only handling, gearbox or protection instead of overwriting everything. | Original proposal. Native part changes can rebuild a vehicle, so arbitrary fields can't be assumed independently applicable. | Medium / medium |
| P1 | Native tuning-variable browser | Expose model-supported tire pressure, alignment, suspension, brake balance and final drive where the installed parts actually provide them. | Variable titles, categories, units, real bounds and defaults are documented [S3]. Don't guess universal variable names or confuse display bounds with actual bounds. Investigate respawn requirements first. | Large / medium to high |
| P1 | Configuration detail cards | Display installed parts summary, available native tuning and available author-supplied power/weight/drive/transmission information beside previews. | Config info fields are documented [S1]. Author metadata isn't measured live performance, especially after ABM edits; label its origin. | Medium / low |
| P1 | Local tags and collections | User-defined Drift / Drag / Towing / Weird builds collections, notes and recent presets, independent of missing mod metadata. | Original proposal; store only ABM metadata, leaving native config/mod files untouched. Existing favorites and built-in filters stay unchanged. | Small to medium / low |
| P1 | Safer preset import/export | Share settings and optional small preview without bundling proprietary vehicle meshes, paid mods or executable code. | Original proposal using native config references [S1]. Validate schema, paths, sizes, finite values and missing dependencies; never automatically install mods. | Medium / medium |
| P2 | Config-linked paint presets | Save named three-slot color/material combinations alongside a car preset. | Three paint slots are documented, but some skins/vehicles use only one or two [S1, S2]. Actual live-paint API and appearance readback still need validation. | Medium / medium |
| P2 | Test-session notebook and replay references | Attach a preset/configuration reference, notes and existing replay file reference to an experiment for later comparison. | Replay `.rpl` files are simulation recordings, not videos, and can break after vehicle/mod/game changes [S4]. Replay-control integration is unverified; start with references, not automatic recording or “export MP4.” | Medium / low for notes, higher for game control |

### Recommended order

1. Verify native save/reload behavior and exact preview lookup on a small stock/custom/modded sample. Include a corrupt image and a missing mod. Don't infer support from one stock car.
2. Build the per-config sidecar library, the loading-on-request flow and the compatibility report together. Make the differences between **Save**, **Load**, **ordinary reset persistence** and **Full reset** obvious.
3. Resolve image fallbacks and add provenance/cache controls. Only then look into optional single-config capture.
4. Add selective preset loading and native tuning variables after application ordering and reset boundaries are proven.
5. Hold paint extras and replay integration until the core save/load/preview flow is reliable.

### Critic's view

- A giant feature list isn't the next release specification. The strongest next slice is **trustworthy presets + trustworthy previews**, not another big group of experimental physics controls.
- A “god-mode config” that quietly loads as an ordinary car without ABM would break what the user expects. Label native-only saves and ABM-required saves clearly.
- Blind auto-loading on vehicle selection is risky. It can change a new session, conflict with another mod or hit a restricted server. Start with loading on user action. Automatic behavior needs its own opt-in policy and fresh permission checks.
- “Native variable available” doesn't mean “safe to live-apply.” Part/native tuning changes may need a rebuild. Don't apply power-slider behavior to every field.
- Preview generation is a simulation operation, not a harmless filesystem read. Don't steal camera focus, replace a car or switch maps in a multiplayer session for a picture.
- Treat compatibility and accurate failure reporting as features. Don't claim universal presets, universal god mode, exact thumbnail capture or measured performance until you've shown them.

## 5. BeamMP player-list redesign, profiles, ID copying and latency

**Requested next work. Not implemented by this research:**

- Improve the BeamMP player list visually: compact aligned rows, a readable name/profile area, an obvious local-player marker, distinct ID and latency fields, vehicle count, and clear empty/disconnected states. Keep ABM's Basic/dark styling and plain-text names.
- Clicking a player's profile/name area should open their real BeamMP user profile in the default browser.
- **Right-clicking a player should copy their BeamMP account ID to the system clipboard right away**, with a small “Copied BeamMP ID” confirmation. Don't replace this gesture with a context menu as the only way to copy. Provide a keyboard-accessible equivalent too.
- Fix the local latency display that shows a dash while BeamMP's own UI shows a value. This is a missing or wrong data-source issue to investigate and fix, not a promise to reduce actual network latency.

### 5.1 Important distinction: session player ID versus account ID

**Installed-source evidence [S7]:** `MPVehicleGE.lua:931-997` builds players with `playerID`, `name` and `isLocal`. `UI.lua:144-164` puts that `player.playerID` into the in-game player-list `id` field. This is the server/session player identifier, not proof of a permanent account ID. ABM currently drops this ID when it builds its player rows.

**Documented [S6]:** `MP.GetPlayerIdentifiers(player_id)` is a **server-side** API that returns identifiers including `beammp`. BeamMP documents this as the permanent forum/account ID, separate from the changeable username. The docs also name a public lookup: `https://forum.beammp.com/u/USERNAME.json`, with the numeric ID at `user.id`. Don't assume a desktop client can call server-side identifier APIs, and don't request private IP/Discord identifiers just to open a profile.

**Installed-source evidence [S7]:** the existing BeamMP player-list UI opens `https://forum.beammp.com/u/<player name>/summary` in `ui/modules/apps/BeamMP-PlayerList/app.js:362-364`. That establishes the current username-based profile route. We didn't verify an account-ID-only browser route, so don't invent one.

Proposed identity flow:

1. Carry the actual session player ID and unformatted raw username through the bridge and receiver. Use session-bound row keys so duplicate names or reordered rows don't misidentify a player.
2. Resolve the permanent account ID from a trusted client field if a future BeamMP version exposes one. Otherwise investigate the documented public profile JSON endpoint. Validate the response's identity, numeric ID and current request target before accepting it.
3. Keep separately labeled `sessionPlayerId` and `beammpAccountId`. Right-click must copy the requested permanent account ID, not quietly substitute the small session ID. A separately named “Copy session ID” action may help, but it must not be confused with the account ID.
4. Open a fixed HTTPS BeamMP forum URL with the raw username encoded as one path segment, not the role-decorated display name or an arbitrary URL from a server. No shell-command construction.
5. Guests, missing/deleted profiles, lookup failures and unresolved IDs need clear states. If the account ID isn't ready, show “Resolving BeamMP ID…” or an error. Never claim a successful copy or put an unrelated ID on the clipboard.

Public-profile lookups add another external network request. Disclose it, resolve on user intent instead of bulk-fetching every player on each refresh, set time and response-size limits, coalesce in-flight requests, cache carefully and respect rate limits. Bind async results to the original server/session/player/name tuple. IDs can be reused after disconnects. Never scrape private identifiers or save a player-tracking history as a side effect.

### 5.2 Why the local latency currently becomes a dash

We inspected the installed BeamMP archive read-only. The following are source findings, not a live connected-game test:

| Stage | Actual source path | Finding |
| --- | --- | --- |
| Player object initialization | `MPVehicleGE.lua:990` | `Player.ping` starts at `-1`, a sentinel for unavailable. |
| Other-player ping updates | `positionGE.lua:93-98` | Received position data updates the owner's `ping` and calls `UI.setPlayerPing(owner.name, ping)`, converting seconds to milliseconds. This doesn't tell us anything about local session ping. |
| Local session latency | `MPCoreNetwork.lua:573-575` | A launcher `p` update during an MP session calls `UI.setPing(data)` and `positionGE.setPing(data)`. |
| BeamMP's visible value | `UI.lua:213-217` | `UI.setPing` emits `onBeamMPSetPing` and stores the value in its private `pings` table under the local nickname. |
| Session UI consumer | `ui/modules/apps/BeamMP-Session/app.js:116-122` | The visible session indicator listens directly to `onBeamMPSetPing`. |
| ABM's current assumption | `game/lua/ge/extensions/beamdeck.lua:564-574` | ABM reads each `p.ping`, drops negative values and derives its session ping from the local player's same field. It doesn't read the separate local session update above. |

**Diagnosis:** the local player object's unavailable `-1` can sit there untouched while the local session UI gets valid latency through `UI.setPing`. ABM then drops `-1` and shows a dash. We need a bridge to the actual local-session source. Another guess at `Player.ping` won't be enough. The installed `UI` module exports setters, but we found no public `getPing` getter in the inspected exports.

Future fix requirements:

- Prefer a supported getter/subscription if BeamMP exposes one. Otherwise design a narrow compatibility adapter for the actual `UI.setPing`/UI-event path, not polling an API that doesn't exist. If you need a wrapper, preserve the original function, arguments, return behavior and normal UI delivery. Restore it only while ABM still owns it, and avoid stacking wrappers across reloads. Don't broadly monkey-patch all `guihooks.trigger` calls or modify installed BeamMP files.
- Keep local session latency separate from remote-player position-derived latency. Use matching source labels internally. Don't average remote values or substitute an operating-system ICMP ping for the session indicator.
- Carry finite, nonnegative milliseconds and freshness/session identity through GE → receiver → backend → Session page and bar popup. Zero is valid. Missing, negative, NaN and stale samples aren't zero. Clear samples on disconnect, joining a new server and bridge reload.
- Match the raw session value used by `onBeamMPSetPing`. Don't invent a correction. The older player-list JS contains subtraction/clamping code, but that doesn't prove the session value needs a universal `-16 ms` adjustment.
- The launcher already requests ping updates about once per second (`MPCoreNetwork.lua:682-685`). Don't send extra probes every ABM telemetry frame just to fill the display.
- If the current BeamMP version can't be observed safely, show a diagnostic reason for “unavailable” instead of a permanent unexplained dash or a made-up number.

### 5.3 Acceptance evidence before calling this complete

- Compare ABM's local latency with BeamMP's live Session UI in the same connected session, including a stationary/spectating player with no spawned vehicle. Check the dashboard and popup, not only a mocked player object.
- Check zero, negative/unavailable, malformed and stale ping. Disconnect/reconnect and server changes must not keep another session's value.
- Confirm left-click opens the correct profile and right-click copies the **permanent account ID**, not the session player ID, without also opening the browser. Check clipboard failure and unavailable IDs as they actually behave.
- Check guest players, Unicode/URL-special names, identical display names, role tags, join/leave while a lookup is pending, reused session IDs, profile errors and lookup rate limits.
- Screenshot the redesigned list at narrow/default widths and with long names, missing ping and many players. Keep row interaction keyboard accessible and dynamic names plain text.
- Preserve all official-server restrictions. Player/profile/latency viewing is read-only and must not imply permission to modify another player's vehicle.

Priority: **P0 for correcting local latency and carrying real identity; P1 for the list redesign and profile/clipboard interactions.** This section records the requested fix and a source-backed diagnosis. No latency or player-list code was changed in this task.

## 6. Further brainstorming: make ABM a useful workshop instead of more sliders

These are **original proposals**, not more verified BeamNG APIs. Some refine themes in the larger roadmap. Consolidate them rather than implementing twice. We did no new web/API verification for this brainstorming section. Priority means suggested ordering, not an approved release commitment.

### 6.1 Trust and control before more physics

| Idea | Concrete interaction | Value / important limit |
| --- | --- | --- |
| **Changed-from-baseline drawer** | List every ABM-owned change on the current car: before, requested, accepted, and verified value where readable. Each row identifies its subsystem and reset behavior. | Makes “what did I change?” answerable. Never label a queued command as applied, or imply a value is verified when only an acknowledgement came back. |
| **Why is this disabled?** | Every unavailable action gives a short reason: unsupported device, moving car, stale bridge, policy lock, missing readback or another action in progress. | More useful than adding controls that silently do nothing. Keep reasons keyboard accessible, not hover-only. |
| **Recover one subsystem** | Separate actions for restoring ABM-owned engine, handling or gearbox changes, beside the existing Full reset. | Full reset stays the escape hatch. Some restores need a physics reset/rebuild. Say so, and don't promise universal undo or repair. |
| **Per-car adapter health** | A compact report showing detected engine/clutch/gearbox adapters, supported readback, conflicting hooks and game/bridge compatibility. | Distinguish “unsupported” from “broken.” Don't change the car just to test support. |
| **Preview a preset transaction** | Before loading, show the exact changes, unsupported fields and whether a reset/respawn is needed. Make partial application a choice you make. | Prevent accidental replacement of a working setup. Native part changes and runtime overrides need a defined order. |
| **Recoverable save history** | Keep a capped revision history for ABM-owned presets, with duplicate-before-edit and restore-previous-version. | Useful protection against bad experiments. No silent backups of other people's mod assets or unlimited disk growth. |

### 6.2 More useful engine and gearbox experimentation

| Idea | Concrete interaction | Value / important limit |
| --- | --- | --- |
| **Before/after engine curves** | Plot captured baseline and current supported torque curves together, with units and limiter markers. Show holes or missing coverage instead of smoothing them into invented power. | Explain why raising the limiter may not add usable power. A sampled device curve isn't a measured chassis dyno result. |
| **Gear ladder / RPM-drop chart** | Visualize each forward ratio and the theoretical RPM after shifting, alongside the existing numerical rows. | Extends actual ratio readback and Shift RPM instead of replacing them. Actual shifts also depend on clutch slip, controller behavior and load. |
| **Target-speed gearing calculator** | When final drive and effective rolling radius can be read reliably, estimate speed versus RPM per gear. Offer an editable target for the top gear. | Refuse calculation when drivetrain reduction or tire data is unknown. Mark estimates and don't pretend geometric tire size equals loaded rolling radius. |
| **Explain +Gear suggestions** | Show which previous ratios informed the suggestion and allow a manual target retention factor. | Makes the existing heuristic easier to understand. Don't rename it “optimal” or claim it maximizes acceleration without a validated model. |
| **Optional power ramp** | Let the user ask for a gradual transition to a new multiplier rather than an instantaneous jump, with Cancel and a visible target. | A proposal for finer control, not an arbitrary power ceiling. Requires latest-value scheduling with clear limits and immediate cancellation on target, permission or connection change. |
| **Clutch stress view** | Display available clutch temperature, thermal efficiency and protection state near the clutch control. Distinguish overheating, reduced capacity and unsupported failure causes. | Helps diagnose the user's real breakage complaint. Don't invent a remaining-life percentage or claim protection increases torque capacity. |

### 6.3 Fun, limited experiments

| Idea | Concrete interaction | Value / important limit |
| --- | --- | --- |
| **Seeded tune randomizer** | Choose supported categories and user-defined ranges, preview a reproducible random setup, then apply it yourself. Offer a saveable seed and a baseline recovery path. | Good sandbox fun without arbitrary scripts. No automatic random respawns, resource refills or out-of-range geometry. Permission and capability checks still apply to every generated value. |
| **Experiment cards** | Save a local recipe such as “low-grip braking,” “long gearing” or “tiny-car test,” with prerequisites, changed values and recovery instructions. | A guided layer over existing actions, not another independent tuning engine. Don't advertise examples as validated safe setups. |
| **A/B setup trials** | Associate two preset snapshots with a manually armed comparison session. Label results with car/config, map, tune revision and simulation speed. | Builds on the earlier test-session proposal. No automatic switching mid-corner or certified timing claims. Resets, teleports and tune changes can invalidate a run. |
| **Body scaling overview** | Show requested width/length/height multipliers and a clear list of what wasn't rescaled: mass, tire behavior, suspension targets, props or controllers. | Helps users reason about odd handling. Don't present calculated physical dimensions as measured values without real bounds readback, and don't imply peer synchronization in MP. |
| **Single-player conditions lab** | Set up supported world settings with a before/after summary, such as wind, gravity or time, separately from the saved vehicle preset. | Consolidates existing world-control ideas. Verify each adapter first. Global conditions must not be changed silently on multiplayer servers or loaded by a car preset. |

### 6.4 BeamMP features beyond a nicer list

| Idea | Concrete interaction | Value / important limit |
| --- | --- | --- |
| **Latency history beyond a single number** | After fixing the actual local source, show a short session-only graph and variation between received samples. Distinguish stale updates from high latency. | Call it sample variation, not measured packet loss or formal network jitter unless the measurement contract supports those claims. No extra per-frame network probes. |
| **Player quick-find and stable sorting** | Filter by name or known account/session ID. Sort by name, vehicles or available ping without moving rows under the pointer. Keep an obvious “You” marker. | Preserve the exact profile/copy gestures requested above. Missing ping sorts consistently. Joining/leaving and reused IDs must not target the wrong person. |
| **Copy a clean server invitation** | Copy the current server's display name and verified address/port, or a supported join link if one is documented later. | Only when the user asks. Don't invent URI schemes, leak passwords/authentication tokens or send chat messages automatically. |
| **Manual convoy notes** | Let users keep temporary local notes about an outing and copy a short car/preset summary for friends when they choose. | No automatic player tracking, persistent account dossiers or scraping. Sharing a preset doesn't synchronize geometry or grant server permission. |
| **Policy status panel** | Explain which exact-name rule and fresh classification currently allow or block ABM modifications, while keeping player/profile information available read-only. | Don't label a roster match “server-admin approved,” certified fair play or anti-cheat protection. Existing policy weaknesses and cleanup behavior stay visible. |

### 6.5 Desktop usability that earns its space

- **Adaptive wide layout:** keep the compact window, but offer an optional wider view for side-by-side current/edited values, gear graphs and preset comparison instead of stretching empty panels. No mandatory giant window.
- **Typed command palette:** searchable actions such as Open Garage, Copy my BeamMP ID, Save preset or Restore gearbox, with the same disabled reasons and confirmations as their buttons. No arbitrary Lua console or shell execution.
- **Named desktop workspace layouts:** remember panel size, filter choices and visible diagnostics separately from car tuning. Restoring a window layout must never apply a tune.
- **Privacy-aware support bundle:** export selected ABM/bridge versions, adapter status and recent errors with a fixed size limit, with a preview/redaction step. Leave out authentication tokens, private paths, server passwords and unrelated player data by default. No automatic upload.
- **Keyboard and low-distraction mode:** predictable focus order, readable table headers, copy shortcuts, reduced optional animation and short status messages. Keep labels legible instead of relying only on color.

### 6.6 What I would actually build next

**Next slice:** correct local ping, accurate account/session identity, reliable preview fallback and a small named preset library. These fix current complaints directly.

**Then:** changed-from-baseline drawer, compatibility explanations and selective preset loading. They make every later experiment easier to understand and recover from.

**First fun extension:** a preview-first seeded randomizer using only settings we already support. It adds variety without dragging in turbo, drivetrain or mass-rebuild adapters.

**Defer:** automatic optimization, unrestricted physical scaling, universal engine swaps, real-time suspension rebuilding and automatic MP tune synchronization. These need a lot of new validation, server cooperation where relevant, and stronger recovery semantics. Another slider and a guessed API won't cover it.

## 7. Sources and verification boundaries

All links below were read on **2026-11-10**. Documentation can lag the installed game. We didn't exercise any game API, native save/load, placeholder path, thumbnail generation or replay-control operation in this research task.

- **S1: BeamNG, Making configurations and info files:** <https://documentation.beamng.com/modding/vehicle/tutorials/configs/>. Config fields, info metadata, generated thumbnails and image-size guidance. Page last modified 2023-08-17.
- **S2: BeamNG, Creating new vehicle configs:** <https://documentation.beamng.com/tutorials/creatingconfigs/>. Part dependencies, per-part tuning availability, three paint slots, custom-config visibility and screenshot-on-save behavior. Page last modified 2025-03-29.
- **S3: BeamNG, Variables:** <https://documentation.beamng.com/modding/vehicle/sections/variables/>. Native tuning metadata, category/subcategory, units, defaults, actual bounds and display bounds. Page last modified 2026-10-19.
- **S4: BeamNG, Replay:** <https://documentation.beamng.com/getting_started/replay/>. `.rpl` semantics, recording/playback and game/mod compatibility warnings. Page last modified 2025-03-29.
- **S5: BeamNG forum, How to use thumbnail generator for a SINGLE configuration?:** <https://www.beamng.com/threads/how-to-use-thumbnail-generator-for-a-single-configuration.96407/>. Community discussion from January 2024. Workflow lead only, not an API guarantee.
- **S6: BeamMP server scripting reference, `MP.GetPlayerIdentifiers`:** <https://docs.beammp.com/scripting/server/latest-server-reference/#mpgetplayeridentifiersplayer_id-number-table>. Distinguishes server player IDs from permanent BeamMP/forum IDs and documents username-based public profile JSON lookup. Read 2026-11-10. This server API isn't assumed available client-side.
- **S7: Installed BeamMP client source, inspected read-only on 2026-11-10:** `~/.local/share/BeamNG/BeamNG.drive/current/mods/multiplayer/beammp.zip`; files `lua/ge/extensions/{MPVehicleGE,MPCoreNetwork,UI,positionGE}.lua` and `ui/modules/apps/{BeamMP-PlayerList,BeamMP-Session}/app.js`. Line references above describe the inspected installation and can change in later versions. No source files were copied into ABM or modified. Public upstream project: <https://github.com/BeamMP/BeamMP>.

Scope stays **Markdown research only**. We didn't do any app implementation, image generation, source/config changes, builds, tests, installation, commits, pushes or release changes for this research addition.
