# RPM, clutch protection, and reset recipes

Development version: **0.10.0**. These additions build on the [workshop foundation](workshop-foundation.md). They are not in the published 0.8.0 preview. Install all bridge files and restart BeamNG when convenient. ABM doesn't restart the game for you.

## Maximum RPM

The Engine group reads the current limiter and accepts a whole-number RPM target for supported combustion engines. Every detected engine must support the adapter. The target must be at least 100 RPM above each engine's idle and within the reported common ceiling, with an absolute input limit of 30,000 RPM.

The ceiling is the higher of the engine's captured original limiter and its existing positive torque-curve coverage, capped by that input limit. Raising the limit does **not** create torque beyond the existing curve. A car whose useful curve ends at the stock limiter won't gain a higher supported limit. Unknown or custom limiter implementations, electric motors, and incompatible multi-engine setups are refused instead of edited on a guess.

RPM-to-angular-velocity fields and the supported limiter's dependent soft range update together. Physical over-rev damage thresholds, engine strength, temporary ESC and two-step limiters, and gearbox shift points don't change. A lower limiter doesn't prevent a mechanically forced over-rev on a downshift. Stock automatic shifting may still happen before the selected limit. Changing RPM is explicit. Only engine power is live-applied.

## Clutch protection

**Clutch ON** targets supported `frictionClutch` devices, not every clutch-like component. It freezes their thermal update, clears that update's accumulated friction-loss input, holds captured wear, and blocks the adapter's deformation-damage hook. OFF restores the hooks owned by ABM. Already damaged or thermally degraded clutches are refused. Repair or reset them first if you want to protect them.

This doesn't increase the clutch's torque capacity, prevent ordinary capacity-related slip, protect the whole drivetrain, or promise numerical stability. DCT, automatic, and custom clutch implementations are not implied. Other mods that replace the same hooks can cause a compatibility refusal. The toggle reports vehicle readback instead of assuming a click worked.

## Ordinary reset versus full reset

**Keep tuning on reset** starts ON for a loaded vehicle extension. Successfully accepted persistent actions are stored in that vehicle VM's memory: power and beam limits, engine strength and protection, RPM, clutch protection, handling, manual ratios, and dimensions. OFF forgets the recipe and cancels pending restoration. It doesn't undo values that were already applied. Turning it back ON records future accepted edits, not every current value from other mods.

On an ordinary physics reset, ABM first restores its captured adapter baselines and advances the generation token. After the game's reset, the extension waits for at least two graphics updates, a short readiness interval, and a fresh GE authorization lease. Ratios and dimensions also require an actually stopped vehicle. Then it attempts the accepted recipe **once**, in a fixed order. It doesn't reapply every frame or retry failed geometry forever.

Restoration has a ten-second graphics-update deadline. Missing authorization, a moving car, or an incompatible device can leave restoration suspended. The UI reports status and errors. A restoration failure can happen after earlier adapters already applied. This isn't an atomic physics transaction. Stop, inspect, reset, or use Full reset instead of assuming every value reverted. Live-game reset timing and experimental geometry still need compatibility testing.

The recipe belongs to the same loaded vehicle extension, not to a saved JBeam, config, or portable preset. A respawn or parts change that creates a new extension doesn't inherit it. Spawn, repair, bounce, fire, tire damage, resources, and world gravity are not replayed. A model or parts change must not be treated as permission to restore an old recipe on a new car.

**Full reset**, at the bottom of Car, clears the recipe and pending restoration before restoring ABM adapters and requesting a stopped physics reset. It keeps loaded custom configuration files and saved presets. World gravity stays separate. A successful reply confirms that a reset was requested, not that a clean physics frame completed. The same partial-restoration limits apply.

## Multiplayer and authorization

The exact-name official-server policy is unchanged. Reset restoration needs a short, connection-bound grant from GE. Blocked or unverified sessions cancel recipes and restoration. Policy cleanup takes priority over persistence. Recipes are never a permission bypass, and they don't synchronize modified geometry to other players. Non-official servers can impose their own rules.

## Gear overview

The editor still reads actual signed gear ratios and keeps reverse and neutral read-only. Its **Shift RPM** column estimates engine RPM after shifting into that row from the preceding edited forward ratio, assuming the shift starts at the reported limiter:

`afterShiftRPM = limiterRPM × nextRatio / previousRatio`

It doesn't model clutch slip, controller delay, wheel speed, final drive changes, or torque and drag optimization. Missing data shows as a dash. **+ Gear** remains an editable median-progression suggestion, not a claim of the best possible ratio.

## Native validation status

The BeamNG MCP server now responds, but its simulator connection failed and its Lua bridge reported a missing token. The development session didn't launch a game, install tokens or mods, drive a car, or run live tuning or reset actions. Installed game sources and isolated tests support the implementation. They don't certify real-game behavior.

## Development validation

One local build, test, and review pass ran: **33 Qt tests passed**, including RPM bounds, target-bound RPM, clutch and persistence commands, and real-QML control gating. The Node plugin/filter, headless CLI, and standalone receiver lifecycle suites passed. The Lua suite stopped at an obsolete assertion that expected the removed periodic GE rewrite, before reaching the new reset-recipe scenarios. The assertion was corrected. The Lua suite and final changes weren't rerun, following the requested no-loop workflow. The full post-fix suite is therefore not confirmed green.

Four demo views (Engine, Gearbox, Body, and policy-locked Car) were inspected under `/tmp/opencode/abm-010-audit`. Small windows scroll to lower actions. Final review fixes preflight clutch-hook ownership before any multi-device mutation, cancel queued power while restoring, revoke the previous selected vehicle's grant, and avoid treating a rounded-down but still-valid server lease as a policy denial. These fixes remain unverified in a live simulator. Pre-existing corrupt installed T-series preview warnings are separate from the application layout.
