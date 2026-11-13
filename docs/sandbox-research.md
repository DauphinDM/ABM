# Sandbox research: next fun controls

These are **researched ideas, not features that exist yet**. Engine strength and the current Car workshop are implemented separately. The research used the installed game's exported Lua APIs. Proprietary game source isn't bundled. Vehicle mods and game updates can change these APIs. No arbitrary-console endpoint is proposed.

## Best next additions

| Priority | Feature and fun use | Verified API or implementation anchor | Limits and design |
| --- | --- | --- | --- |
| 1 | Named presets: ice drift, sticky tires, moon buggy, fragile engine | Existing ABM actions | Save requested settings locally, preview changes, and restore the captured stock values. No invented physics readback. |
| 2 | Slow motion and crash-lab controls | `simTimeAuthority.setTargetSpeed` | The game clamps speed to 0.001-1×, so this can't fast-forward. World-wide and single-player only, clearly labeled. |
| 3 | Sunrise, night, and accelerated days | `core_environment.getTimeOfDay` / `setTimeOfDay` | Preserve unrelated time fields and restore the previous state. Map lighting support differs, and the effect is world-wide. |
| 4 | Wind tunnel and crosswind challenges | `core_environment.setGroundWind` / `setWindSpeed` | Affects every vehicle. Use a limited vector and an explicit reset. Start with single-player. |
| 5 | Refill or drain fuel, nitrous, battery, and air | Energy-storage devices' `setRemainingRatio` in `vehicle/energyStorage/{fuelTank,n2oTank,electricBattery,pressureTank}.lua` | Detect each storage type first. Filling a tank changes mass. Don't promise infinite energy or a universal nitrous power slider. |
| 6 | Cruise speed and acceleration experiments | `vehicle/extensions/cruiseControl.lua`: `setSpeed`, `setEnabled`, `setTargetAcceleration` | The existing controller still governs throttle. It's not a guaranteed speed or teleport. Cancel on braking or reset. |

## Higher-risk experiments

- **Weight and balance:** the node-mass API `obj:setNodeMass` is used by `vehicle/extensions/gameplayInterfaceModules/interactCargoContainers.lua`. Scaling a whole vehicle also needs inertia and wheel/suspension compatibility. Require a stopped vehicle, finite positive values, and reversible baselines.
- **Per-wheel pressure, suspension, aero, turbo, and rev-limit editors:** these are desirable, but there's no universal safe mutation contract yet. Inspect each supported device and controller before enabling a control. Never edit a display field and claim you changed the physics.
- **Random challenge generator:** build it from supported presets, not random unbounded values. Show the exact changes and give the user one-click restoration.

## Common guardrails

Keep edits scoped to the selected, locally owned vehicle. World controls should be single-player only unless the server supports them. Reject non-finite or overflowing values and partial multi-device changes. Report what the vehicle VM actually completed. Make it clear which edits are runtime-only and resettable, and which are saved vehicle configurations. Presets, capability discovery, and stock restoration are the best foundation for broader customization.

## Research references

- Installed game Lua: `lua/vehicle/powertrain/combustionEngine.lua` (durability), `lua/ge/simTimeAuthority.lua`, `lua/ge/extensions/core/environment.lua`, and the vehicle paths above. The APIs were read, not inferred from console folklore.
- Community inspiration: [global vehicle values editor](https://www.beamng.com/resources/vehicle-global-strength-weight-size-and-other-values-edit-works-for-mods.14868/).
- Distribution choice: [AppImage concepts](https://docs.appimage.org/introduction/concepts.html), [Qt Linux support](https://doc.qt.io/qt-6.8/linux.html), and the [linuxdeploy Qt plugin](https://github.com/linuxdeploy/linuxdeploy-plugin-qt). See [portable Linux](portable-linux.md). Broad distro support is possible. Literal all-distro support is not.
