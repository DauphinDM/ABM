# ABM next-update implementation plan

**Status: full feature scope requested; waiting for your go-ahead to implement.**

Date: 2026-11-10. Planning baseline: ABM 0.10.0.

Inputs: [next feature requests and research](next-feature-requests.md), [larger update research](massive-update-research.md), and [current reset-persistence design](reset-persistence.md).

The user has requested **everything in this plan**, including all later slices. Phases define implementation order. They don't make scope optional and they don't give permission to drop features. Delivery can be staged without asking you to pick features again. Don't claim the full update is complete until every requested item is implemented and validated, or until a real blocker is reported and you agree to a changed outcome. The research document is still the evidence register. New class names, contracts and limits below are proposals, not existing APIs.

## 1. Full scope and first implementation milestone

Deliver four connected improvements:

1. **Accurate BeamMP latency**, including the dashboard and bar popup.
2. **A better player list**, with correct profile links, separate session/account identity and right-click account-ID copying.
3. **Reliable config/mod previews**, including corrupt-image fallback and clear image provenance.
4. **A small named ABM preset library**, saved per vehicle/configuration and loaded only after compatibility checks.

Supporting work includes structured disabled reasons, enough accepted-state readback to save presets accurately, and a staged application coordinator. These are dependencies, not an excuse to rebuild the whole app.

The complete scope also includes **every feature in Slices E through K**, not only the four initial improvements. The native tuning-variable editor belongs to its later phase. Native configuration pairing must be investigated and implemented when a safe, verified mechanism exists. Standalone presets are an initial delivery, not grounds to quietly abandon that investigation.

Turbo controls, physical mass changes, engine swaps, automatic optimization, arbitrary console execution, automatic MP tune synchronization and preview rendering are outside this plan's implementation scope. We excluded or deferred them in research. They aren't hidden commitments. Adding everything in this plan doesn't authorize unsafe bypasses or an unsupported API.

The first deliverable can save an **ABM preset referencing a normal BeamNG configuration**, even if native paired saving isn't proven. It must not advertise a self-contained god-mode `.pc` file.

## 2. Non-negotiable boundaries

- Preserve Basic/dark, compact, square Omarchy styling, opaque text and existing translucency. No Material redesign or Danish translation work.
- Preserve the existing official-server exact-name guard, permission freshness, ownership checks and target/generation fencing. Viewing players, profiles and previews stays read-only under a tuning lock.
- Never modify installed BeamMP files or copy its implementation into ABM. Any compatibility adapter must be small, ownership-aware and preserve normal behavior.
- Don't launch/reset/replace a car, move a camera, change a map, install a mod or capture a preview as a side effect of opening a page.
- Never overwrite a native configuration or existing preset without warning. Leave proprietary game/mod assets where they are.
- Saved data holds fixed validated values. No Lua, shell commands, credentials, saved permission grants or obsolete vehicle tokens.
- No automatic tune loading on selection/startup/server connection. Ordinary-reset persistence stays separate from disk presets.
- Keep the repository private. Don't publish a release, change visibility or move existing release tags as part of this work without authorization.

## 3. Architecture and planned file boundaries

Keep C++ responsible for storage, network requests, validation and action orchestration. QML owns layout and gestures. GE owns current-session identity/permission. Vehicle Lua owns supported tuning adapters and readback.

| Area | Existing files to extend | Proposed additions / responsibility |
| --- | --- | --- |
| Latency and player identity | `game/lua/ge/extensions/beamdeck.lua`, `src/telemetry.{h,cpp}`, `src/backend.{h,cpp}` | Source/freshness metadata, strict identity fields, adapter lifecycle and session history with a fixed limit. |
| Player presentation | `src/Main.qml`, `src/resources.qrc` | `src/PlayerRow.qml` for plain-text, accessible row interactions; `src/playerprofiles.{h,cpp}` for fixed-origin lookup/browser/clipboard operations. |
| Previews | `src/previews.{h,cpp}`, `src/backend.cpp`, `src/VariantCard.qml` | Ordered candidate descriptors, verified identity, decode/fallback with fixed limits and provenance; reuse the existing image provider. |
| Preset storage | `src/backend.{h,cpp}`, `src/protocol.h` | `src/presetstore.{h,cpp}` for schema validation, atomic ABM-owned files and local library operations. |
| Preset application | GE bridge, `game/lua/vehicle/extensions/abmTuning.lua`, receiver | `src/presetcontroller.{h,cpp}` for preflight, frozen targets, serialized application and accurate partial-result reporting. Reuse current adapters. |
| Preset UI | `src/Main.qml`, `src/resources.qrc` | `src/PresetPanel.qml`: library, details, compatibility preview and save/load confirmations. |
| Packaging and tests | `abm.pro`, `tests/abm_tests.pro`, `tests/abm_tests.cpp`, `tests/game_test.lua`, `tests/plugin_test.cjs`, `pkgbuild/PKGBUILD` | Register new source/resources, compatibility tests and packaged bridge changes. Preserve existing test entry points. |

Don't split the app into a generic plugin framework. Small dedicated components are enough. Native configuration saving belongs behind a verified game adapter, not a direct desktop writer that guesses `.pc` semantics.

## 4. Gate A: establish the contracts before UI expansion

### A1. Limited verification work

After approval, inspect the installed game/BeamMP versions once for:

- The actual local ping entry point and any supported getter/subscription that replaces the researched `UI.setPing` path.
- Available registered/guest identity information, raw username and session player ID. Don't assume the session ID is a forum account ID.
- Current configuration identity, parts/native-variable descriptors and the safe native save mechanism.
- Mounted preview resolution, user-folder overrides and any real default-placeholder asset.
- Current accepted-recipe/readback coverage. ABM 0.10 doesn't already expose every setting needed for an accurate complete saved preset.

Use read-only MCP inspection if connected and capable. If it isn't available, use installed-source evidence and report native verification as blocked. Don't probe it over and over or pretend mocks are live evidence. Destructive/native save-load experiments need a disposable car/configuration you've agreed to.

### A2. Additive versus mutating protocol changes

- Carry a protocol/capability revision for new preset snapshot/application behavior. Agree the concrete version during implementation.
- New mutating operations need a bridge that's verified as compatible and refuses mixed versions. Preserve supported older read-only behavior where feasible.
- A command target is the current session, owned vehicle ID, vehicle generation, relevant configuration identity and request/intent revision. Never save this target to disk.
- A saved preset has a separate schema version and adapter requirements. Don't equate it with the wire version.
- Keep request, acknowledgement and verified readback separate. An acknowledgement can prove an adapter ran. It can't prove every physical effect or reset has finished.

**Exit gate:** no guessed permanent IDs, placeholder paths, full-preset state or native-save guarantees. Unknowns get clear disabled/fallback states instead of blocking unrelated read-only improvements.

## 5. Slice B: fix latency and player interactions

### B1. Observe the actual session latency

1. Prefer a supported source discovered in Gate A. Otherwise wrap only the researched local setter through a small compatibility adapter.
2. Install once per actual module instance. Preserve original arguments, return behavior and normal UI delivery. Unload/reload restores the original only when ABM still owns the wrapper. Never stack wrappers or overwrite another mod's replacement.
3. Capture finite nonnegative milliseconds with session identity and a monotonic observation timestamp. Keep local session ping separate from remote position-derived player ping.
4. Publish `value`, `source`, freshness and an unavailable reason through GE, receiver and backend. Zero is still a valid sample. Don't make up zero, subtract a guessed correction or send extra per-frame probes.
5. Invalidate right away on disconnect/session change. Expire stale samples using a documented threshold that fits the existing roughly one-second updates. Start with a proposed five-second threshold and adjust only from observed update cadence.
6. Use the same normalized value for Session, dashboard and popup. Add a small in-memory history with a fixed limit only after this source is correct. Call it sample variation, not packet loss.

### B2. Identity and profile resolution

Player rows carry separate fields:

- Stable current-session row key, `sessionPlayerId`, raw username and local/guest state where known.
- Optional `beammpAccountId`, resolution status and identity source.
- Optional ping plus source/freshness, vehicle count and display-only role information.

Prefer a verified account field from a trusted source. If there isn't one, resolve the documented public forum JSON endpoint when the user asks, not by continuously querying every row. Validate the returned canonical identity under documented normalization rules. Don't invent fuzzy username matching. Unknown guest/identity status must not turn into a falsely verified account.

The public lookup gives you a forum-account mapping for a name, **not cryptographic proof that the connected player owns that account**. Make that distinction visible where it matters. Never request IP or Discord identifiers.

Proposed lookup controls: one in-flight request per identity, a small concurrency limit, ten-second request timeout, 256 KiB response cap and memory-only cache with a fixed limit. Handle HTTP errors/rate limits without retry storms. Use normal TLS validation and a fixed HTTPS origin. Reject unexpected redirects. Validate numeric IDs exactly, including JSON precision/overflow, before formatting them as decimal strings.

### B3. Exact gestures and clipboard lifecycle

- **Left click/name activation:** open the fixed BeamMP forum username profile route in the default browser, using one correctly encoded raw-name path segment. Don't construct shell commands or accept server-supplied URLs.
- **Right click:** copy the permanent account ID directly when resolved. Don't open the browser or substitute a context menu/session ID.
- If resolution is needed, show a pending-copy state. Bind the result to a short-lived copy intent that the user asked for, and to the original row/session/name. Cancel when the row/session changes, the request expires, a newer copy intent replaces it or the clipboard changes in the meantime.
- On success, submit through Qt's native clipboard API and show confirmation only for the intent that's still valid. Use readback where meaningful and report platform failures. Never overwrite the clipboard later from an abandoned lookup.
- Guests, unresolved/deleted accounts and lookup failures show a reason. A separately labeled Copy session ID action must stay distinct.
- Keyboard focus, Enter for profile and a clear Copy account ID action provide equivalents. Row sorting must not move a target underneath an active pointer interaction.

### B4. Layout

Use compact aligned rows: name/profile, “You” marker, clearly labeled ID state, latency and vehicle count. Add search and stable sorting without hiding missing data. Use plain text for every dynamic name. Long names elide with an accessible full value. Missing ping gets an explanation, not a permanent unexplained dash.

**Exit evidence:** compare the dashboard and popup with BeamMP's real local Session indicator, including no-car/spectator states. Exercise zero/stale/malformed latency, reconnects, guest/unresolved IDs, Unicode names, delayed lookups and ID reuse. A diagnosis from source alone isn't a completed live fix.

## 6. Slice C: accurate preview fallback and provenance

Extend the existing image provider instead of adding downloads or an image library.

1. Give each catalog/config item an ordered set of image candidates with model/config identity, source revision and provenance.
2. Resolve the exact config image according to actual mounted precedence, including user overrides and mod archives. A filename/display name match alone isn't enough.
3. Try to decode the image instead of only checking that the file exists. On missing, corrupt or unreadable content, advance to the next candidate for that same car.
4. Fall back to its correct model image with **Model preview** labeling, then a verified game placeholder if available, then ABM's compact existing fallback.
5. Publish a status/reason with a fixed size limit for diagnostics: exact, model fallback, placeholder, missing, corrupt, stale or unsupported. Never substitute a different vehicle's photo.
6. Refresh only affected entries when source/config revision changes. Deduplicate concurrent requests and avoid repeatedly decoding known-bad candidates until their revision changes.

Proposed initial budgets: eight candidates per item, a 16 MiB encoded candidate cap, a 16-megapixel decode ceiling and a 64 MiB in-memory thumbnail cache. Check dimensions before full decode where the decoder supports it. Count actual decoded allocation, not only compressed bytes. Limit archive entry scans and decompressed reads. Reject path traversal/symlink escapes outside approved roots and keep opaque provider IDs rather than exposing arbitrary filesystem access.

These numbers are starting limits, not performance claims. Oversized/unsupported images get a clear fallback. Start without a disk thumbnail cache. Add one only if measured cost justifies it, with a clear budget and owned-cache cleanup.

**Exit evidence:** exact custom config, mod archive, user override, absent model image, corrupt JPEG, oversized input and source replacement all resolve correctly. Opening Garage never changes the simulation. Screenshot both successful and missing-image cards.

## 7. Slice D: named per-car/config ABM presets

### D1. Obtain an accurate snapshot

Add a fixed read-only workshop snapshot operation, tied to the current owned vehicle/generation. It returns supported **accepted ABM settings** and their adapter versions, plus the model/config reference and a reproducible compatibility descriptor. Don't save draft text, stale global preferences or guessed stock values.

The descriptor should cover relevant selected parts/native variables and supported device identity. Hash a canonical, sorted representation of verified fields. Define exactly what's included. If full mod-version/config-file identity can't be established, mark compatibility partial rather than claiming an exact match.

Represent unsupported, unknown and “no ABM override recorded” separately. Accepted means ABM accepted an override. It doesn't mean another mod hasn't changed the value afterward. You can offer to save a preset with partial-state disclosure, but never call that a complete clone without saying so.

### D2. Disk schema and storage

Start with standalone files inside an ABM-owned preset directory under its platform configuration/data location. Use opaque generated IDs as filenames, not raw model IDs or user display names. Keep user-facing names as data.

Proposed schema fields:

- `schemaVersion`, preset ID/name and timestamps.
- Model/config reference, compatibility descriptor/hash and verification completeness.
- ABM/adapter requirements and a clear list of included subsystems.
- Validated accepted vehicle settings: supported power, RPM, protection, handling, forward gear ratios and experimental dimensions when the user includes them.
- Reset-persistence preference as a separate option, not an invisible result of saving a preset.

Exclude live vehicle IDs, generation tokens, session/account identity, policy approvals, gravity/world settings, fire/bounce/repair actions, credentials, executable code and proprietary assets.

Use `QSaveFile` for ABM-owned writes, strict schema/finite-number checks and safe canonical paths. Reject unknown future schemas rather than guessing a migration. Start with proposed limits of 128 KiB per preset and 256 entries, including existing per-adapter gear payload bounds. List oversized/invalid entries as errors. Never silently delete them or truncate settings.

Implement Save new, Rename, Duplicate, Update after confirmation and Delete after confirmation. Keep previous data on write failure. Cache only validated metadata with a fixed limit. Don't parse files without a size limit during every QML binding evaluation.

### D3. Compatibility preview and loading

1. Select a preset and read the current fresh snapshot/capabilities. Cancel pending live-power edits before starting a preset transaction so they can't race its writes.
2. Show matching, changed, missing and unsupported subsystems, expected resets and experimental warnings. No silent field substitution.
3. Require exact compatible targeting by default. For a partial load, make the user select a supported subset. Revalidate that subset rather than declaring the whole preset compatible.
4. Freeze the current session/owned vehicle/generation, permission and relevant native configuration identity. Recheck at each boundary and right before vehicle-side mutation.
5. Preflight every selected adapter before writing anything. Serialize application through the existing command boundary. Don't run it alongside live-power or another preset operation.
6. Apply in one documented order: supported native configuration work, if separately enabled, finishes first and produces a fresh generation. Then runtime handling, gearbox, engine/power/RPM/protection and experimental geometry in an adapter-defined order. For the initial runtime-only loader, reuse the existing reset recipe's established order and document dependencies rather than inventing a second ordering system.
7. Read back what is supported, report the exact completed/failed/unknown operations and install only successfully accepted reset-recipe state. A partial preset must not leave a false complete recipe waiting to replay on the next ordinary reset.

Transaction states: `Idle → Preflight → Awaiting confirmation → Applying → Verifying → Complete | Partial | Failed | Cancelled`.

Cancellation removes unsent work. It can't undo a mutation already performed. Target/policy loss halts further work and follows existing policy cleanup. Full reset cancels the transaction and recipe first. Don't promise atomic rollback of physical geometry or repair. Offer a clear recovery path with accurate reset/respawn warnings.

For the first loader, reject application while moving if any selected adapter needs a stopped car. Runtime-only loading must not silently respawn it. Experimental geometry stays opt-in even when the file contains it.

### D4. Native configuration pairing is a conditional follow-up

Investigate a fresh disposable native config save/reload with permission. Verify which native fields survive and whether an associated sidecar can be found reliably. Don't inject arbitrary runtime state into `.pc` just because it accepts JSON.

If proven, add a separate **Save native configuration + ABM preset** action under a new name and a clear **Load configuration + ABM preset** flow. Loading the native file must finish, identify the new generation/configuration and pass all gates before runtime application.

Treat native-save and ABM-save results separately. Two files aren't one atomic transaction. Report partial failure without deleting or overwriting the user's existing config. If this adapter isn't reliable, ship the standalone library and document that limitation rather than blocking the useful first slice.

**Exit evidence:** save accepted state, restart ABM, select the matching configuration and reload it on a disposable test car. Reject stale/wrong targets and show missing adapters/parts. A normal `.pc` load alone must not be presented as restoring ABM-only god mode.

## 8. Required later slices: disposition of every brainstorm group

**All rows below are required scope.** They follow the core milestone because they depend on its state, storage and recovery contracts. None may be silently omitted or treated as an optional extra. An unavailable dependency needs a visible blocker and an agreed resolution, not a fake implementation.

| Later slice | Included ideas | Required foundation / stop condition |
| --- | --- | --- |
| E: explain and recover | Changed-from-baseline drawer, disabled reasons, adapter health, per-subsystem recovery, selective comparison/loading and capped save history. | Verified baseline/accepted-state provenance and preset coordinator. Don't expose a one-click restore for adapters that can't restore safely. Basic disabled explanations and compatibility preview are already required in the first build. |
| F: engine/gear understanding | Before/after curves, gear ladder, explained +Gear/manual retention, target-speed calculator and clutch stress view. | Real device readback with fixed limits. Omit target-speed calculations without effective wheel/drivetrain data. Label curve/shift estimates, not measured dyno output. |
| G: controlled experiments | Seeded randomizer, experiment cards, A/B trials, optional power ramp, body-scaling summary and single-player conditions lab. | Presets, deterministic validation, preview-before-apply and cancellation/recovery. Randomizer uses only supported adapters. No automatic simulation changes or silent global MP settings. |
| H: deeper native setup | Model-supported tuning-variable browser, richer configuration/parts details and three-slot paint presets. | Verify bounds, dependency/rebuild behavior and actual paint support. No guessed universal pressure/alignment variables or live-apply assumptions. |
| I: organization and portability | Local tags/collections, strict import/export and test notebook/replay references. | Stable preset schema and ownership boundaries. No bundled paid/proprietary assets, executable payloads or automatic mod installation. Replays stay references until control APIs are verified. |
| J: multiplayer polish | Latency history, quick-find/stable sorting, server invitation copying, local convoy notes and policy status you can trust. | Correct latency/identity first. No persistent player tracking, invented join URI or claims that a roster match is admin approval. Basic search/sorting and policy explanations can come with Slice B. |
| K: desktop ergonomics | Adaptive wide layout, fixed-action command palette, workspace layouts, redacted support bundle, keyboard/low-distraction mode. | Keep compact defaults, shared action guards and privacy. A workspace restore must never load a car preset. No arbitrary console hidden in the palette. |

The original request for presets and accurate images isn't displaced by these extras. Existing live power, RPM, clutch protection, reset persistence and Garage filters are maintenance dependencies, not features to reannounce as newly built.

## 9. Validation and evidence plan: limited, not a loop

After implementation is authorized and complete:

1. Run the relevant automated suites once, including new cases below and existing regression coverage.
2. Do one combined code/security/UI review and a planned set of screenshots. Fix concrete findings and stop. Don't repeatedly search/test/review for more work.
3. Record exactly which commit/state each result covers. If review fixes change behavior after testing, say they weren't retested. Don't label the final state green. Unresolved critical behavior or missing native evidence is a shipping gate, not permission for an endless loop. Request authorization for more verification if needed.

Planned coverage:

| Area | Minimum cases |
| --- | --- |
| Latency | Zero/negative/NaN, stale/missing sample, no vehicle, reconnect, source/module replacement, wrapper ownership and unchanged normal BeamMP UI behavior. |
| Players | Registered/guest/unknown, Unicode/path-special names, invalid/mismatched profile JSON, oversized/error/rate-limited response, late results, reused IDs, cancelled copy, clipboard failure and profile gesture separation. |
| Previews | Exact custom/mod image, override precedence, missing/corrupt/oversized entry, archive/path rejection, fallback with fixed limits and source invalidation. |
| Storage | Round-trip schema, invalid/unknown fields, nonfinite numbers, truncated files, traversal names, write failure, overwrite/delete confirmation and migration refusal. |
| Application | Wrong model/config/parts, stale generation, policy loss, moving car, paused/expired request, live-power race, mid-transaction failure, recipe cancellation and Full reset priority. |
| UI | Default/narrow/wide windows, long names, many rows, keyboard gestures, pending/errors, missing previews, partial preset status and no new QML warnings. |

Use deterministic local fixtures for network/schema/image errors. Don't bulk-query real player profiles as tests. Live checks use permission you've been given and disposable configuration names. Capture demo screenshots without private server/player data for documentation.

Native evidence should include stock/custom/modded vehicles plus clear unsupported-device behavior. Real preset mutation/save/reload and latency comparison are separate from mocked tests. If game/MCP access is unavailable, say so and don't claim native success.

## 10. Packaging, compatibility and delivery gate

- Keep Linux native-package, portable Linux and Windows build paths intact. New Qt classes/resources must be registered everywhere they compile.
- Clipboard/browser and path handling need Windows as well as Linux coverage. Offscreen CI doesn't establish desktop clipboard behavior or real-game compatibility.
- Update bridge installation/version requirements and explain whether BeamNG must restart. Never restart the user's game automatically.
- Document network behavior for public profile lookups, storage locations, cache limits, preset schema and known compatibility limits.
- Report which tests passed, which fixes weren't retested and what's still unverified. Don't ship a claimed complete native-save or latency fix on screenshots alone.
- Installation, private push and release creation are separate delivery choices. Creating this plan authorizes none of them. Existing tags stay untouched.

## 11. Full-scope completion and authorization

**The plan covers the entire requested update: Gate A and every Slice B through K.**

Completion must account for all of these groups:

- Correct latency, profile opening, permanent account-ID copying, player list/search/sorting and accurate identity states.
- Accurate preview resolution/fallback, provenance and cache behavior with fixed limits.
- Named presets, compatibility preview, loading on request, native-save pairing investigation and safe integration where verified.
- Changed-state drawer, disabled explanations, adapter health, subsystem recovery, selective comparison/loading and save history.
- Engine curves, gear ladder, explained/manual +Gear suggestions, target-speed gearing and clutch stress.
- Seeded randomizer, experiment cards, A/B trials, power ramp, body-scaling overview and single-player conditions lab.
- Native tuning variables, configuration/parts details and supported paint presets.
- Tags/collections, validated import/export and notebook/replay references.
- Latency history, server invitation copying, convoy notes and policy status you can trust.
- Adaptive wide layout, fixed-action command palette, workspace layouts, redacted support export and keyboard/low-distraction support.

Implement core dependencies first, then go through all later groups. Staged delivery is fine. Label a partial milestone as partial instead of presenting it as the completed full update. Keep an item-by-item implementation/evidence ledger during the build so no table row or desktop-usability bullet gets lost.

Full scope doesn't mean universal vehicle support, universal god mode or permission to bypass server rules. Required features must refuse unsupported vehicles and say why. Where native behavior really isn't available, report the specific evidence, proposed fallback and affected requirement to the user before treating a different outcome as accepted.

**Current action: scope updated to everything above. Waiting for your go-ahead to implement. This clarification changes only the planning Markdown file.**
