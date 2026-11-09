# Sandbox research — next fun controls

These are **researched proposals, not implemented features**. Engine strength
and the existing Car workshop are implemented separately. Research used the
installed game's exported Lua APIs; proprietary game source is not bundled.
Vehicle mods/game updates can change these APIs. No external arbitrary-console
endpoint is proposed.

## Best next additions

| Priority | Feature / fun use | Verified API or implementation anchor | Limits and design |
| --- | --- | --- | --- |
| 1 | Named presets: ice drift, sticky tires, moon buggy, fragile engine | Existing ABM actions | Save requested settings locally, preview changes, restore captured stock. No invented physics readback. |
| 2 | Slow motion / crash-lab controls | `simTimeAuthority.setTargetSpeed` | Game clamps speed to 0.001–1×; not fast-forward. World-wide, single-player only, clearly labeled. |
| 3 | Sunrise/night and accelerated day | `core_environment.getTimeOfDay` / `setTimeOfDay` | Preserve unrelated time fields, restore previous state. Map lighting support differs; world-wide. |
| 4 | Wind tunnel / crosswind challenges | `core_environment.setGroundWind` / `setWindSpeed` | Affects every vehicle. Bounded vector and explicit reset, single-player first. |
| 5 | Fuel, nitrous, battery and air refill/drain | Energy-storage devices' `setRemainingRatio` in `vehicle/energyStorage/{fuelTank,n2oTank,electricBattery,pressureTank}.lua` | Capability-detect each storage type. Filling a tank changes mass; do not promise infinite energy or a universal nitrous power slider. |
| 6 | Cruise speed and acceleration experiments | `vehicle/extensions/cruiseControl.lua`: `setSpeed`, `setEnabled`, `setTargetAcceleration` | Existing controller still governs throttle; not a guaranteed speed/teleport. Cancel on braking/reset. |

## Higher-risk experiments

- **Weight/balance:** node-mass API `obj:setNodeMass` is used by
  `vehicle/extensions/gameplayInterfaceModules/interactCargoContainers.lua`.
  Whole-vehicle scaling also needs inertia and wheel/suspension compatibility;
  require a stopped vehicle, finite positive values and reversible baselines.
- **Per-wheel pressure, suspension, aero, turbo and rev-limit editors:** desirable,
  but no universal safe mutation contract has been established here. Inspect
  each supported device/controller before enabling a control. Never just edit a
  display field and claim it changed the physics.
- **Random challenge generator:** compose supported presets rather than random
  unbounded values; show the exact changes and provide one-click restoration.

## Common guardrails

Keep edits scoped to the selected locally owned vehicle. World controls should
be single-player only unless the server explicitly supports them. Reject
non-finite/overflowing values and partial unsupported multi-device changes;
report actual VM completion. Distinguish resettable runtime edits from saved
vehicle configurations. Presets + capability discovery + stock restoration
are the best foundation for broader customizability.

## Research references

- Installed game Lua: `lua/vehicle/powertrain/combustionEngine.lua` (durability),
  `lua/ge/simTimeAuthority.lua`, `lua/ge/extensions/core/environment.lua`, and the
  vehicle paths above. APIs were read, not inferred from console folklore.
- Community inspiration: [global vehicle values editor](https://www.beamng.com/resources/vehicle-global-strength-weight-size-and-other-values-edit-works-for-mods.14868/).
- Distribution choice: [AppImage concepts](https://docs.appimage.org/introduction/concepts.html),
  [Qt Linux support](https://doc.qt.io/qt-6.8/linux.html), and
  [linuxdeploy Qt plugin](https://github.com/linuxdeploy/linuxdeploy-plugin-qt).
  See [portable Linux](portable-linux.md): broad distro support is possible,
  literal all-distro support is not.
