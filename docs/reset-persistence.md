# RPM, clutch protection and reset recipes

Development version: **0.10.0**. These additions build on the [workshop foundation](workshop-foundation.md); they are not in the published 0.8.0 preview. Install all bridge files and restart BeamNG when convenient. The app does not restart the game for you.

## Maximum RPM

The Engine group reads the current limiter and accepts a whole-number RPM target for supported combustion engines. All detected engines must support the adapter. The target must be at least 100 RPM above every engine's idle and within the reported common ceiling, with an absolute input bound of 30,000 RPM.

The ceiling is the greater of the engine's captured original limiter and its existing positive torque-curve coverage, capped by that input bound. Raising the limit does **not** create torque beyond the existing curve. A car whose useful curve ends at its stock limiter will not acquire a higher supported limit. Unknown/custom limiter implementations, electric motors and incompatible multi-engine arrangements are refused rather than edited speculatively.

RPM-to-angular-velocity fields and the supported limiter's dependent soft range are updated together. Physical over-rev damage thresholds, engine strength, temporary ESC/two-step limiters and gearbox shift points are not changed. A lower limiter does not prevent a mechanically forced over-rev on a downshift. Stock automatic shifting may still occur before the selected limit. Changing RPM is explicit; only engine power is live-applied.

## Clutch protection

**Clutch ON** targets supported `frictionClutch` devices, not every clutch-like component. It freezes their thermal update, clears that update's accumulated friction-loss input, holds captured wear and blocks the adapter's deformation-damage hook. OFF restores hooks owned by ABM. Already damaged/thermally degraded clutches are refused: repair/reset them first if desired.

This does not increase the clutch's torque capacity, prevent ordinary capacity-related slip, protect the whole drivetrain or promise numerical stability. DCT/automatic/custom clutch implementations are not implied. Other mods replacing the same hooks can cause a compatibility refusal. The toggle reports vehicle readback rather than assuming a click succeeded.

## Ordinary reset versus full reset

**Keep tuning on reset** starts ON for a loaded vehicle extension. Successfully accepted persistent actions are stored in that vehicle VM's memory: power/beam limits, engine strength/protection, RPM, clutch protection, handling, manual ratios and dimensions. OFF forgets the recipe and cancels pending restoration; it does not undo already applied values. Turning it back ON records future accepted edits, not every current value from other mods.

On an ordinary physics reset, ABM first restores its captured adapter baselines and advances the generation token. After the game's reset, the extension waits for at least two graphics updates, a short readiness interval and a fresh GE authorization lease. Ratios or dimensions additionally require an actual stopped vehicle. It then attempts the accepted recipe **once**, in a fixed order. It does not reapply every frame or retry failed geometry indefinitely.

Restoration has a ten-second graphics-update deadline. Missing authorization, a moving car or an incompatible device can leave restoration suspended; the UI reports status/errors. A restoration failure can occur after earlier adapters already applied: this is not an atomic physics transaction. Stop, inspect/reset or use Full reset rather than assuming all values reverted. Live-game reset timing and experimental geometry still require compatibility validation.

The recipe is scoped to the same loaded vehicle extension, not a saved JBeam/config or a portable preset. A respawn/parts change that creates a new extension does not inherit it. Spawn, repair, bounce, fire, tire damage, resources and world gravity are not replayed. A model/parts change must not be treated as permission to restore an old recipe on a new car.

**Full reset**, at the bottom of Car, clears the recipe and pending restoration before restoring ABM adapters and requesting a stopped physics reset. It preserves loaded custom configuration files and saved presets. It leaves world gravity separate. A successful reply confirms reset was requested, not that a clean physics frame completed. Existing partial-restoration limits still apply.

## Multiplayer and authorization

The exact-name official-server policy remains unchanged. A short, connection-bound grant from GE is required for reset restoration. Blocked/unverified sessions cancel recipes and restoration; policy cleanup takes priority over persistence. Recipes are never a permission bypass and do not synchronize modified geometry to other players. Non-official servers can impose their own rules.

## Gear overview

The editor still reads actual signed gear ratios and keeps reverse/neutral read-only. Its new **Shift RPM** column estimates engine RPM after shifting into that row from the preceding edited forward ratio, assuming the shift begins at the reported limiter:

`afterShiftRPM = limiterRPM × nextRatio / previousRatio`

Clutch slip, controller delay, wheel speed, final drive changes and torque/drag optimization are not modeled. Missing data is a dash. **+ Gear** remains an editable median-progression suggestion, not a claim of the best possible ratio.

## Native-validation status

The BeamNG MCP server now responds, but its simulator connection failed and its Lua bridge reported a missing token. The development session did not launch a game, install tokens/mods, drive a car or run live tuning/reset actions. Installed game sources and isolated tests support the implementation; they do not certify real-game behavior.

## Development validation

One local build/test/review pass: **33 Qt tests passed**, including RPM bounds, target-bound RPM/clutch/persistence commands and real-QML control gating. Node plugin/filter, headless CLI and standalone receiver lifecycle suites passed. The Lua suite stopped at an obsolete assertion expecting the removed periodic GE rewrite, before reaching the new reset-recipe scenarios. That assertion was corrected; the Lua suite and final changes were not rerun, following the requested no-loop workflow. The full post-fix suite is therefore not confirmed green.

Four demo views (Engine, Gearbox, Body and policy-locked Car) were inspected under `/tmp/opencode/abm-010-audit`. Small windows scroll to lower actions. Final review fixes preflight clutch-hook ownership before any multi-device mutation, cancel queued power while restoring, revoke the previous selected vehicle's grant and avoid treating a rounded-down but still-valid server lease as a policy denial. These fixes remain unverified in a live simulator. Pre-existing corrupt installed T-series preview warnings remain separate from the application layout.
