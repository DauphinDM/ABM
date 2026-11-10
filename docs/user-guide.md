# ABM user guide

> **Development 0.9:** [Workshop foundation](workshop-foundation.md) replaces the old power Apply step with live power, adds actual per-gear readback/editing and a bottom-of-Car full reset. Bridge 0.9.0 and a game restart are required. Older versioned descriptions below remain historical where they describe the previous ratio text box. Editable maximum RPM, clutch protection and reset-persistent recipes are not implemented yet.

## Official-server guard (development 0.7.1)

Multiplayer gameplay changes require bridge 0.7.1 and verified server permission.
The frozen 108 regular official names are allowed by exact raw name only;
rotating their IP/port does not remove approval. Official event/new or unapproved names are blocked;
verified non-official servers have no extra ABM restriction. Unknown status
locks changes. A banner explains the decision. Telemetry/browsing and Stop
bouncing remain available. Permission loss stops reapply and requests cleanup,
which **can reset your car while moving**. See [policy, privacy and important
limitations](server-policy.md); this is not anti-cheat or staff protection.

## Launch and connection state

The main button starts BeamNG through Steam when the game is closed. When the
game is already running it can enable the installed BeamMP mod through the
bridge and start BeamMP with `--no-launch`, avoiding a second game instance.
BeamMP must already be installed. A running launcher is not a server connection.

**Game running · waiting for bridge** means process detection succeeded but
telemetry has not arrived. Do not assume the game crashed. Install/load the
bridge and check diagnostics instead. The feed expires after three seconds
without fresh telemetry.

## Drive

Speed uses world velocity, not driven-wheel speed; wheelspin should not inflate
the reading. Gauges interpolate on a 60-update/second target while the game
feed samples at up to 20 Hz. This does not force the renderer/game to 60 FPS.
Fuel, coolant and boost stay unavailable if the vehicle does not supply them.

The speed history is bounded in memory. **Vehicle details** opens extended
diagnostics. **Options** changes units, opens bridge setup, pauses the display
or opens diagnostics. Pausing is local display behavior, not a game pause.

## Car: editing and sandbox actions

Select your own active vehicle in BeamNG. The four workshop sections are:

- **Engine & protection**: power, beam immunity, engine strength and protection.
- **Handling & gearbox**: grip, brakes, steering, ESC and manual ratios together.
- **World & recovery**: gravity/presets, bounce start/stop, repair, fire and tires.
- **Body · experimental**: width, length and height, with explicit risk consent.

Install all updated bridge files and restart BeamNG when ready. The wire version
remains **0.7.1**, but older copies do not report the new capability flags. Controls
stay disabled until the selected vehicle reports support. Gear apply, repair and
dimensions require a known stopped speed; invalid handling/ratio inputs disable
Apply. Strength is disabled while engine immunity is on. Server policy remains
authoritative; **Stop bouncing** remains available when other actions are locked.
Values are requested drafts, not measured live settings. Apply the relevant group
explicitly; engine protection and planet presets still apply immediately.

| Control | Range / behavior |
| --- | --- |
| Engine power | Slider 50–300%; numeric entry accepts any finite positive percentage, scaling supported engines' runtime torque curves; rejects derived overflow |
| Engine strength | 0.1–100× combustion-engine torque tolerance and over-torque/over-rev damage budgets; bridge 0.6 required |
| Engine indestructible | Block supported engine damage hooks and freeze thermal updates; not whole-vehicle godmode |
| Gravity | −30 to 0 m/s²; affects all world objects |
| Planet presets | Earth, Moon, Mars, Venus, Jupiter, Pluto, zero gravity; apply immediately |
| Beam protection | Raises deformation/strength thresholds; disabling restores stock thresholds |
| Repair | Attempts repair in place; vehicle must be stopped (≤1 m/s) |
| Fire / Extinguish | Queue the game's vehicle fire functions |
| Deflate tires | Queues the game's tire-deflation function |
| Grip / sliding grip | Independently scale intact tire tread static/sliding friction, 0–10× |
| Brakes | Scale service and parking brake torque, 0–10× |
| Steering response | Scale steering hydro input from its baseline, 0.1–3×; physical limits remain |
| ESC / traction mode | Cycle the supported vehicle ESC controller's existing modes |
| Forward gear ratios | Space/comma/semicolon-separated ratios, 0.01–100 each; supported manual gearboxes only |
| Why This Dealer? / Stop bouncing | Start with `vehicle:why("this dealer")`, stop with `vehicle:why("stop")`; Stop needs bridge 0.6.1, no reset needed |
| Dimensions | Experimental independent width/length/height, 0.5–2× captured shape; stopped, owned car; BeamMP requires bridge 0.6.2 and is local only |

### Handling and extra gears (development 0.5)

Apply handling explicitly; the values shown start at stock **1×** and are not
live readback. Tire damage and thermal/custom controllers may override changes.
Deflated/broken tires are skipped, not repaired. Reset restores stock handling.

The gearbox editor accepts more than the factory gear count. **+ Gear** appends
a progressively taller ratio; you can edit/paste the entire list. There is no
fixed gear-count cap, but a **24 KB serialized ratio-array budget** bounds each
request. This is not mathematically unlimited. Automatic, DCT and CVT devices
are deliberately unsupported. Stop first (≤1 m/s); applying selects neutral,
rebuilds indexed synchronizer/sound and controller shift-point tables from the captured baseline, and may
restore captured synchronizer wear. Use sequential shift controls for added
gears. Reset the vehicle to restore its stock gearbox.

**Bridge 0.6.2 is required for all gear edits.** Earlier bridges left the
controller's shift-point table at its factory size, causing a nil-entry exception
and vehicle shutdown when arcade mode reached an added gear. Added entries now
inherit stock top-gear thresholds. This uses named Lua closure introspection to
reach the game's internal getter (not a stable public API); unsupported/modded
controllers are refused before mutation. Reset a previously disabled vehicle
after updating. Driving the fix in the real game remains to be verified.

The meme button uses the real easter egg, not a replacement impulse. Builds
without that method report it unavailable. New vehicle actions report the
vehicle VM's response; this confirms script execution, not measured handling.
Actual in-game effects still need gameplay validation.

Repeated power changes scale the saved base curve rather than compounding the
previous multiplier. Not every engine/motor/mod has the same powertrain API;
unsupported devices may not change. This is not an ECU editor or a horsepower
measurement. Higher torque can cause overheating or mechanical failure.

Beam protection is **not complete godmode**: tires, powertrain components and
other systems can still fail. It cannot repair existing deformation.

### Physical dimensions (development 0.6.2)

Car → **Body · experimental** exposes three independent sliders. Accept
the physics warning, stop bouncing and stop the vehicle, then Apply. BeamMP now
allows your **own car only** with bridge 0.6.2. Broken beams require repair/reset first.
**Resized geometry is local, not synchronized to peers.** Other players may see
stock dimensions and collisions can disagree. Use on consenting/private servers,
follow their rules, and reset if behavior diverges. ABM does not install a server
mod, broadcast resizing, edit other players' cars, or bypass server restrictions.

The first apply captures the current physical node shape and beam rest lengths.
Further applies are relative to that capture, not compounded. Width/length use
the car's own orientation; height scales around its captured lowest node. Beam
rest lengths follow endpoint scaling while preserving their captured preload.
This uses the same node/rest-length/commit APIs as the game's state loader, not
an editor-only visual scale. **1× returns to the captured shape, not necessarily
an undamaged factory shape. Reset the vehicle for stock geometry.**

This is an experimental sandbox deformation, not a complete vehicle rebuild:
mass, tire radius/pressure, hydro targets, controllers, props and drivetrain
parameters are not rescaled. Suspension may fight the change, meshes may distort,
and the vehicle may become unstable or damaged. No saved JBeam/config is edited.
The API path and math are mock-tested; actual game physics remain unverified.

Bounce can also be stopped immediately from the game's **GE Lua** console:
`be:getPlayerVehicle(0):why("stop")`, provided a vehicle is selected and the
game exposes the easter egg. The ABM Stop button checks availability/ownership.

### Engine strength and protection (development 0.6)

Choose **Car → Engine & protection**. Move the strength slider and press **Apply
strength**. The **Indestructible engine** toggle applies immediately. These
values are requested settings, not live durability measurements.

Strength scales the captured stock torque rating and damage budgets, not
horsepower, the rev limiter, or the whole vehicle. Stock negative torque-limit
sentinels remain unlimited. Only the verified combustion-engine API is supported;
electric/custom motors report unsupported instead of silently pretending.

Protection blocks the supported engine lock-up/break/deformation/flooding hooks,
disables over-torque/over-rev failures, and **freezes thermal simulation**. It
does not repair existing damage, refill fuel, stop unrelated controllers, or
protect tires, mounts, turbochargers and the rest of the drivetrain. It is an
engine-only sandbox mode, not a guarantee of an indestructible car. Turning it
off restores normal hooks at the selected strength. **Stock limits**
sets 1× with protection off; a vehicle reset also restores these engine edits.
Effects still need real-game validation.

### Restoring normal physics

Apply **100% engine power**, **−9.81 m/s² gravity**, and **protection off**.
Factory reset buttons are available after an accepted tuning request. Settings
periodically re-apply to the same active vehicle while the extension is loaded;
pressing the game's reset key alone is not a reliable way to disable them.
Switching vehicles does not intentionally transfer a tune to another vehicle.
Previously modified vehicles may retain changes until restored/reset.

The app remembers the last accepted requested slider values across launches.
Those values are not a live readback and are not automatically applied just
because the app starts. Gravity is world-wide and can outlive a vehicle change.

Car actions are explicit and may damage your current vehicle/session. Unlike
garage spawning, these fun buttons act immediately. Do not use them to test
connectivity. Multiplayer ownership guards reject another player's selected
vehicle; respect server rules, and expect servers to override local actions.

## Garage

1. Load a map and let the catalog populate.
2. Search/filter models and click a photo to see its configurations.
3. Combine **All content**, **Custom configs**, **Mod content** or **Stock content**
   with a vehicle type (Car, Truck, Trailer, Prop, etc., as reported by the game).
4. Star a model/configuration to persist it as a favorite.
5. Select a configuration and choose **Spawn** or **Replace**.
6. Review the confirmation before accepting; replace discards the current car.

The **★** filter is independent: favorites can be combined with all other filters.
**More filters** adds brand, body style, configuration type, drivetrain,
transmission, exact source/mod name, country, fuel type and propulsion. Choices
come from the current model/configuration catalog, not a hardcoded vehicle list.
Filters with no metadata are disabled. **Clear** resets search and filters for
the current level; model and configuration filters are remembered separately.

Sort by name A–Z/Z–A, brand, type, source or favorites first. Search includes
metadata and descriptions, so mod and config details are searchable too.
At model level a facet means **any configuration** has that value: selecting
Manual and Diesel does not prove one variant has both. Open the model to narrow
individual configurations. Models can contain both stock and custom/mod configs,
so content categories can overlap. Missing metadata is never guessed. The updated
bridge files are required for full metadata even though the wire version is unchanged.

Photos come from loose game files or installed archives. A configuration without
its own photo uses a labeled model fallback, or **No preview**. Custom part
editing and creating new configuration files are not included.

Vehicle switching/spawning/replacing needs a loaded map and speed ≤1 m/s,
checked by both the receiver and game bridge. An acknowledgement means the
request was submitted, not that the game/server accepted the resulting spawn.

## Session and diagnostics

Trip distance, average speed and acceleration/timing values are estimates from
telemetry, not official odometer or race timing. Resets, teleports, unusual
physics and dropped samples can invalidate measurements. Receiver restarts,
map/session changes and stale reconnects reset counters.

Multiplayer shows cleaned server titles, searchable players, available pings
and counts. BeamMP being loaded does not mean you are connected. **Your vehicles**
only lists locally owned entries; selection is confirmed and movement-guarded.
**Reset trip** affects ABM's counters, not your game save.

## Omarchy popup

Left-click starts the game if closed, otherwise toggles the popup. Right-click
opens/focuses ABM. The popup hides irrelevant multiplayer/no-vehicle readings.
Enter opens the dashboard; Escape closes the popup. A duplicate IPC registration
warning with multiple monitor bars is expected: IPC chooses the first registered
widget, while clicking a widget operates that monitor's instance.

## Keyboard shortcuts

| Key | Action |
| --- | --- |
| 1 / 2 / 3 / 4 | Drive / Car / Garage / Session |
| Space | Pause/resume this display |
| U | Toggle km/h and mph |
| ? | Keyboard help |
| Escape | Clear search, return to models or dismiss a dialog/help |
| Q | Close the dashboard |

Search fields retain normal text entry rather than triggering single-letter
shortcuts. On Omarchy the service stays running after closing the dashboard;
in standalone mode the app-owned receiver ends with the app.
