# ABM user guide

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

Select an active vehicle in BeamNG and load bridge 0.5.0 for the full workshop
(0.4 retains its original power/world controls). Choose **Power & world**, **Grip &
handling**, **Gearbox**, or **Fun & recovery**. Adjust **Engine power**,
**Gravity** and **Deformation immunity**, then choose **Apply tuning**.

| Control | Range / behavior |
| --- | --- |
| Engine power | Slider 50–300%; numeric entry accepts any finite positive percentage, scaling supported engines' runtime torque curves; rejects derived overflow |
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
| Why This Dealer? | Call the actual `vehicle:why("this dealer")` easter egg if available; reset the vehicle to stop |

### Handling and extra gears (development 0.5)

Apply handling explicitly; the values shown start at stock **1×** and are not
live readback. Tire damage and thermal/custom controllers may override changes.
Deflated/broken tires are skipped, not repaired. Reset restores stock handling.

The gearbox editor accepts more than the factory gear count. **+ Gear** appends
a progressively taller ratio; you can edit/paste the entire list. There is no
fixed gear-count cap, but a **24 KB serialized ratio-array budget** bounds each
request. This is not mathematically unlimited. Automatic, DCT and CVT devices
are deliberately unsupported. Stop first (≤1 m/s); applying selects neutral,
rebuilds indexed synchronizer/sound tables from the captured baseline, and may
restore captured synchronizer wear. Use sequential shift controls for added
gears. Reset the vehicle to restore its stock gearbox.

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
3. Filter configurations by **All**, **Custom**, or **Favorites**.
4. Star a model/configuration to persist it as a favorite.
5. Select a configuration and choose **Spawn** or **Replace**.
6. Review the confirmation before accepting; replace discards the current car.

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
