# ABM user guide

These notes describe development builds. They are here so older screenshots and reports still make sense.

- **0.11:** session-only BeamMP latency details, player identity and profile actions, limited ABM presets, supported torque and clutch readback, a seeded tune preview, server-address copy, convoy notes, and a redacted support bundle. Install bridge **0.11.0** and vehicle bridge **0.11.0**. Older bridges stay read-only for these controls. Presets store accepted ABM overrides, not native `.pc` files. Applying a preset is checked step by step, without atomic rollback. Native preset save and load remain unverified until they're tested with a disposable car.
- **0.10:** [RPM limits, clutch protection, and same-vehicle reset persistence](reset-persistence.md). Install bridge 0.10.0. An ordinary reset keeps accepted supported recipes when the option is on. **Full reset** clears them. The detailed limits in that guide take priority over older reset descriptions here.
- **0.11.1:** ratio editing now works with manual, automatic, DCT, and sequential transmissions. Those types keep their existing gear count. `+ Gear` remains manual-only. CVT uses one fixed ratio. Older notes below that describe manual-only editing are kept for context.
- **0.9:** [Workshop foundation](workshop-foundation.md) replaced the old power Apply step with live power. It also added actual per-gear readback and editing, plus a full reset at the bottom of Car. Bridge 0.9.0 and a game restart are required. Older versioned descriptions below are historical where they mention the old ratio text box. Editable maximum RPM, clutch protection, and reset-persistent recipes were not implemented yet at that point.

## Official-server guard (development 0.7.1)

Multiplayer gameplay changes need bridge 0.7.1 and verified server permission. Only the frozen 108 regular official names are allowed, matched by exact raw name. Changing their IP or port does not remove approval. Official event names and new or unapproved official names are blocked. Verified non-official servers have no extra ABM restriction. If ABM can't determine the status, changes stay locked. A banner explains the decision. Telemetry, browsing, and **Stop bouncing** still work.

If permission is lost, ABM stops re-applying changes and requests cleanup. **That cleanup can reset your car while you're moving.** See [policy, privacy, and important limits](server-policy.md). This is not anti-cheat or staff protection.

## Dashboard layout

The window opens at 560×840 and can shrink to 320×440. On wide windows, the page tabs become a left sidebar. **Car** opens the Workshop, where the section selector keeps related controls together. Short windows hide the large page headings to leave more room for content. Pages scroll when needed.

Garage cards adapt from one to four columns. **Filters** opens a scrollable dialog. At the smallest height, it also shows content, type, and favorites. Use Tab to focus a card, Enter or Space to open it, and the separate star button to favorite it. Ctrl+K opens the command palette. `?` shows keyboard shortcuts.

Changing the appearance doesn't grant permission to change the game. Ownership, movement, bridge capability, and server-policy checks still apply.

The 0.13.0 release adds a fifth **Connect** tab for [ABM Mobile pairing](mobile-companion.md). Phone access is off by default, unavailable in demo mode, and requires explicit approval on the PC. This tab is not in the older 0.12.0 binaries.

## Launch and connection state

The main button starts BeamNG through Steam when the game is closed. When the game is already running, it can enable the installed BeamMP mod through the bridge and start BeamMP with `--no-launch`, so you don't get a second game instance. BeamMP must already be installed. A running launcher does not mean you're connected to a server.

**Game running · waiting for bridge** means ABM found the game process but telemetry hasn't arrived yet. Don't assume the game crashed. Install or load the bridge, then check diagnostics. The feed expires after three seconds without fresh telemetry.

## Drive

Speed uses world velocity, not driven-wheel speed, so wheelspin shouldn't inflate the reading. Gauges interpolate toward 60 updates per second while the game feed samples at up to 20 Hz. This doesn't force the game or renderer to 60 FPS. Fuel, coolant, and boost stay unavailable if the vehicle doesn't supply them.

The speed history is kept in memory with a size cap. **Vehicle details** opens extended diagnostics. **Options** changes units, opens bridge setup, pauses the display, or opens diagnostics. Pausing affects the display only. It does not pause the game.

## Car: editing and sandbox actions

Select your own active vehicle in BeamNG. The four workshop sections are:

- **Engine & protection:** power, beam immunity, engine strength, and protection.
- **Handling & gearbox:** grip, brakes, steering, ESC, and manual ratios.
- **World & recovery:** gravity and presets, bounce start/stop, repair, fire, and tires.
- **Body · experimental:** width, length, and height, with explicit risk consent.

Install all updated bridge files and restart BeamNG when you're ready. The wire version stays **0.7.1**, but older copies don't report the new capability flags. Controls stay disabled until the selected vehicle reports support. Gear apply, repair, and dimensions need a known stopped speed. Invalid handling or ratio inputs disable Apply. Strength is disabled while engine immunity is on. Server policy remains in charge. **Stop bouncing** stays available when other actions are locked.

The values you see are requested drafts, not measured live settings. Apply the group you want. Engine protection and planet presets still apply immediately.

| Control | Range / behavior |
| --- | --- |
| Engine power | Slider 50-300%. Numeric entry accepts any finite positive percentage and scales supported engines' runtime torque curves. Derived overflow is rejected. |
| Engine strength | 0.1-100× combustion-engine torque tolerance and over-torque/over-rev damage budgets. Bridge 0.6 required. |
| Engine indestructible | Blocks supported engine damage hooks and freezes thermal updates. This is not whole-vehicle godmode. |
| Gravity | −30 to 0 m/s². Affects all world objects. |
| Planet presets | Earth, Moon, Mars, Venus, Jupiter, Pluto, zero gravity. These apply immediately. |
| Beam protection | Raises deformation and strength thresholds. Turning it off restores stock thresholds. |
| Repair | Tries to repair in place. The vehicle must be stopped (≤1 m/s). |
| Fire / Extinguish | Queues the game's vehicle fire functions. |
| Deflate tires | Queues the game's tire-deflation function. |
| Grip / sliding grip | Independently scales intact tire tread static and sliding friction, 0-10×. |
| Brakes | Scales service and parking brake torque, 0-10×. |
| Steering response | Scales steering hydro input from its baseline, 0.1-3×. Physical limits still apply. |
| ESC / traction mode | Cycles the supported vehicle ESC controller's existing modes. |
| Forward gear ratios | Ratios separated by spaces, commas, or semicolons, 0.01-100 each. Supported manual gearboxes only. |
| Why This Dealer? / Stop bouncing | Start with `vehicle:why("this dealer")`, stop with `vehicle:why("stop")`. Stop needs bridge 0.6.1 and no reset. |
| Dimensions | Experimental independent width, length, and height, 0.5-2× the captured shape. Stopped, owned car. BeamMP requires bridge 0.6.2, and resizing is local only. |

### Handling and extra gears (development 0.5)

Apply handling. The values shown start at stock **1×** and are not live readback. Tire damage and thermal or custom controllers may override your changes. Deflated or broken tires are skipped, not repaired. Reset restores stock handling.

The gearbox editor accepts more than the factory gear count. **+ Gear** appends a progressively taller ratio, and you can edit or paste the whole list. There is no fixed gear-count cap, but each request is limited to a **24 KB serialized ratio-array budget**. That's not mathematically unlimited. Automatic, DCT, and CVT devices are not supported here on purpose. Stop first (≤1 m/s). Applying selects neutral, rebuilds indexed synchronizer, sound, and controller shift-point tables from the captured baseline, and may restore captured synchronizer wear. Use the sequential shift controls for added gears. Reset the vehicle to restore its stock gearbox.

**Bridge 0.6.2 is required for all gear edits.** Earlier bridges left the controller's shift-point table at its factory size. That caused a nil-entry exception and shut down the vehicle when arcade mode reached an added gear. Added entries now inherit stock top-gear thresholds. This uses named Lua closure introspection to reach the game's internal getter, which is not a stable public API. Unsupported or modded controllers are refused before anything changes. If a vehicle was disabled by this bug, reset it after updating. Driving the fix in the real game still needs verification.

The meme button uses the real easter egg, not a replacement impulse. Builds without that method report it unavailable. New vehicle actions report the vehicle VM's response. That confirms the script ran, not that handling was measured. Actual in-game effects still need gameplay testing.

Repeated power changes scale the saved base curve instead of compounding the previous multiplier. Not every engine, motor, or mod has the same powertrain API. Unsupported devices may not change. This is not an ECU editor or a horsepower measurement. Higher torque can cause overheating or mechanical failure.

Beam protection is **not complete godmode**. Tires, powertrain components, and other systems can still fail. It can't repair existing deformation.

### Physical dimensions (development 0.6.2)

Car → **Body · experimental** has three independent sliders. Accept the physics warning, stop bouncing, stop the vehicle, then Apply. BeamMP now allows your **own car only** with bridge 0.6.2. Broken beams need repair or reset first. **Resized geometry is local and not synchronized to other players.** They may see stock dimensions, and collisions can disagree. Use it on consenting or private servers, follow their rules, and reset if behavior goes wrong. ABM does not install a server mod, broadcast resizing, edit other players' cars, or bypass server restrictions.

The first apply captures the current physical node shape and beam rest lengths. Further applies are relative to that capture, not compounded. Width and length use the car's own orientation. Height scales around its captured lowest node. Beam rest lengths follow endpoint scaling while keeping their captured preload. This uses the same node, rest-length, and commit APIs as the game's state loader, not an editor-only visual scale. **1× returns to the captured shape, not necessarily an undamaged factory shape. Reset the vehicle for stock geometry.**

This is an experimental sandbox deformation, not a complete vehicle rebuild. Mass, tire radius and pressure, hydro targets, controllers, props, and drivetrain parameters are not rescaled. Suspension may fight the change, meshes may distort, and the vehicle may become unstable or damaged. No saved JBeam or config file is edited. The API path and math are mock-tested. Actual game physics still need verification.

You can also stop bouncing from the game's **GE Lua** console with `be:getPlayerVehicle(0):why("stop")`, as long as a vehicle is selected and the game exposes the easter egg. The ABM Stop button checks availability and ownership.

### Engine strength and protection (development 0.6)

Choose **Car → Engine & protection**. Move the strength slider and press **Apply strength**. The **Indestructible engine** toggle applies immediately. These are requested settings, not live durability measurements.

Strength scales the captured stock torque rating and damage budgets. It does not change horsepower, the rev limiter, or the whole vehicle. Stock negative torque-limit sentinels stay unlimited. Only the verified combustion-engine API is supported. Electric and custom motors report unsupported instead of pretending to work.

Protection blocks the supported engine lock-up, break, deformation, and flooding hooks. It disables over-torque and over-rev failures, and it **freezes thermal simulation**. It does not repair existing damage, refill fuel, stop unrelated controllers, or protect tires, mounts, turbochargers, or the rest of the drivetrain. It's an engine-only sandbox mode, not a guarantee that the car can't be destroyed. Turning it off restores normal hooks at the selected strength. **Stock limits** sets 1× with protection off. A vehicle reset also restores these engine edits. Effects still need real-game validation.

### Restoring normal physics

Apply **100% engine power**, **−9.81 m/s² gravity**, and **protection off**. Factory reset buttons are available after an accepted tuning request. Settings re-apply periodically to the same active vehicle while the extension is loaded. Pressing the game's reset key alone is not a reliable way to disable them. Switching vehicles does not transfer a tune to another vehicle. Previously modified vehicles may keep changes until you restore or reset them.

The app remembers the last accepted slider values across launches. Those values are not live readback, and ABM does not apply them just because it starts. Gravity is world-wide and can outlive a vehicle change.

Car actions are explicit and can damage your current vehicle or session. Unlike garage spawning, these fun buttons act immediately. Don't use them to test connectivity. Multiplayer ownership guards reject another player's selected vehicle. Respect server rules, and expect servers to override local actions.

## Garage

1. Load a map and let the catalog populate.
2. Search or filter models, then click a photo to see its configurations.
3. Combine **All content**, **Custom configs**, **Mod content**, or **Stock content** with a vehicle type (Car, Truck, Trailer, Prop, etc., as reported by the game).
4. Star a model or configuration to save it as a favorite.
5. Select a configuration and choose **Spawn** or **Replace**.
6. Review the confirmation before accepting. Replace discards the current car.

The **★** filter is independent. You can combine favorites with any other filter. **More filters** adds brand, body style, configuration type, drivetrain, transmission, exact source/mod name, country, fuel type, and propulsion. Choices come from the current model and configuration catalog, not a hardcoded vehicle list. Filters with no metadata are disabled. **Clear** resets search and filters for the current level. Model and configuration filters are remembered separately.

Sort by name A-Z or Z-A, brand, type, source, or favorites first. Search includes metadata and descriptions, so mod and config details are searchable too. At model level, a facet means **any configuration** has that value. Selecting Manual and Diesel does not prove that one variant has both. Open the model to narrow individual configurations. Models can contain both stock and custom/mod configs, so content categories can overlap. Missing metadata is never guessed. The updated bridge files are required for full metadata even though the wire version is unchanged.

Photos come from loose game files or installed archives. If an image is missing, corrupt, unreadable, or too large, ABM tries another local candidate for the same image, then the configuration's model fallback, then **No preview**. **Model preview** appears only when that fallback actually loads. No images are downloaded, and opening Garage doesn't change the simulation.

Decoded thumbnails and failures use a memory-only cache with a size limit. Refresh the catalog or configurations after replacing an image. Registering it again checks the candidate files' paths, sizes, and modification times. Restart ABM if an external tool replaced an image but kept all of that metadata the same. Custom part editing and creating new configuration files are not included.

Vehicle switching, spawning, and replacing need a loaded map and speed ≤1 m/s, checked by both the receiver and the game bridge. An acknowledgement means the request was submitted, not that the game or server accepted the spawn.

## Session and diagnostics

Trip distance, average speed, and acceleration/timing values are estimates from telemetry, not official odometer or race timing. Resets, teleports, unusual physics, and dropped samples can invalidate measurements. Receiver restarts, map or session changes, and stale reconnects reset the counters.

Multiplayer shows cleaned server titles, searchable players, available pings, and counts. BeamMP being loaded does not mean you're connected. **Your vehicles** only lists locally owned entries. Selection is confirmed and guarded by movement. **Reset trip** affects ABM's counters, not your game save.

Player buttons do different things:

- **Copy name / Copy details** copy the current session row, with the result shown on the card. **Session ID** is a connection-local ID, not a permanent account ID.
- **Profile** opens the public BeamMP forum username page in your browser. **Forum ID** requests that username's public forum JSON record. It needs internet access and a matching forum username. This is not proof of in-game identity. Guests with explicit guest status have neither action.
- A server or player change, unavailable telemetry, or another clipboard write cancels a pending forum-ID copy. Errors are shown without substituting a session ID. Missing session IDs alone do not prove guest status.
- **Copy server address** copies only the reported endpoint, with brackets for IPv6. It doesn't create an invitation or copy a password.

In `--demo`, name, details, and session-ID copying use sample data. Local dialogs still open. Public-profile requests, switching, and trip reset are disabled. The owned-vehicles dialog explains why selection is disabled in a real session. These source fixes need the updated GE bridge and a game restart before they affect live BeamMP ownership or roster behavior.

API references and the isolated verification scope are in [Session action review](session-actions.md).

## Omarchy popup

Left-click starts the game if it's closed. Otherwise it toggles the popup. Right-click opens or focuses ABM. The popup hides multiplayer readings and no-vehicle readings when they don't apply. Enter opens the dashboard, and Escape closes the popup.

A duplicate IPC registration warning with multiple monitor bars is expected. IPC chooses the first registered widget, while clicking a widget operates that monitor's instance.

## Keyboard shortcuts

| Key | Action |
| --- | --- |
| 1 / 2 / 3 / 4 | Drive / Car / Garage / Session |
| Space | Pause or resume this display |
| U | Toggle km/h and mph |
| ? | Keyboard help |
| Escape | Clear search, return to models, or dismiss a dialog or help |
| Q | Close the dashboard |

Search fields keep normal text entry instead of triggering single-letter shortcuts. On Omarchy, the service keeps running after you close the dashboard. In standalone mode, the app-owned receiver stops with the app.
