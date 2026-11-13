# ABM massive update: research and proposed roadmap

Research date: **2026-11-10**. Baseline: **ABM 0.8.0**, with the current 0.7.1 bridge protocol.

**Status: planning only.** We haven't implemented, installed, or gameplay-tested any feature in this document as part of this research. The only repository change for this task is this new Markdown file. It adds to [the earlier sandbox research](sandbox-research.md); it doesn't replace it.

## 1. Main conclusions

1. **Make engine power live, but fix command handling first.** If you wire a slider straight into the current request path, edits get dropped while the backend is busy, and a delayed command can land on the wrong vehicle. Use a dedicated engine-power command bound to a target, with a latest-value queue.
2. **Treat maximum RPM as several interacting limits, not one number.** The combustion-engine limiter, the available torque curve, physical over-rev damage, temporary ESC/launch limits, and transmission shift logic are separate systems.
3. **Use the vehicle's own tuning-variable metadata for broad customization.** That lets you expose supported suspension, alignment, tire, drivetrain, and ECU settings without inventing universal property names. Many changes need a rebuild, so they can't behave like live power sliders.
4. **Build presets, readback, and recovery before you add dozens of dangerous controls.** Without them, users can't reliably understand, reproduce, or undo a tune.
5. **Keep multiplayer policy and ownership checks under every new action.** More features must not weaken the existing official-server guard. They also shouldn't suggest that unrestricted community servers grant admin permission.
6. **Ordinary vehicle resets should keep accepted tunes.** Grip, gearing, dimensions, and other supported tuning should come back automatically on the same compatible car. A separate full-reset button at the bottom of the Car tab must clear that behavior and restore the loaded configuration's baseline.
7. **Make the gearbox readable and add clutch protection.** Read the actual installed ratios, show each gear clearly, and suggest an added ratio from the existing progression. Look into clutch-specific protection instead of assuming engine protection covers it.

The strongest next update isn't a bigger pile of cheat buttons. It's a **capability-aware tuning workbench with live controls, staged rebuild settings, reproducible experiments, and clear recovery**.

## 2. Evidence and boundaries

Evidence labels used below:

- **[D] Documented:** we read public upstream documentation.
- **[L] Local source:** we inspected the relevant installed BeamNG Lua read-only. That gives us an implementation lead for this installation. It doesn't prove a stable public API or correct gameplay behavior.
- **[C] Community:** a public mod author advertises the feature. Treat it as inspiration, not an independently verified implementation, safety claim, or compatibility claim.
- **[P] Proposal:** ABM-specific reasoning or design work. Not implemented yet.

We didn't install or run any community mod for this research. Don't copy proprietary game source into ABM. Public documents can lag behind the installed game. Modded engines and controllers may behave differently.

### What is already present, not a new feature

ABM already has engine-power scaling, beam protection, engine-strength/protection controls, handling multipliers, experimental manual-gear editing, experimental dimensions, gravity presets, recovery/fun actions, garage metadata filters, capability gating, and an official-server policy. The proposals below extend or reorganize these features. Don't market them as newly invented equivalents.

## 3. Requested feature: maximum RPM

### 3.1 What the installed engine actually does

We inspected `lua/vehicle/powertrain/combustionEngine.lua` in the installed BeamNG directory. Line numbers are research anchors, and they'll drift between versions.

| System | Observed behavior | Consequence for ABM |
|---|---|---|
| Permanent operating limit | The constructor sets `maxRPM`, `maxAV`, `revLimiterRPM`, and `revLimiterAV`. Limiter type can be `soft`, `timeBased`, or `rpmDrop` (around lines 1721 and 1962 to 1989). | Updating one field isn't enough. Each supported limiter needs a consistent adapter and a way to restore stock. |
| Temporary limiter | `setTempRevLimiter()` takes angular velocity in rad/s. Limiter implementations use the lower of the permanent and temporary limits (around 641 to 691). | This setter can't raise the permanent redline. It isn't a general maximum-RPM API. |
| Torque availability | Physics reads an integer-RPM entry from `torqueCurve` and falls back to zero when it's absent (around 746). Curve construction and redline falloff happen around 2008 to 2030. | Raising the limiter doesn't create usable torque above the original curve, and it doesn't guarantee the engine reaches the RPM you asked for. |
| Mechanical tolerance | `maxPhysicalAV` controls over-rev damage, separately from the throttle limiter (around 500 and 1886). | A higher limiter can damage the engine. A limiter also can't prevent every mechanically forced over-rev during a downshift. |
| Other controllers | ESC and two-step launch control also set and reset temporary rev limits. | ABM must not fight them with a periodic temporary-limiter override. |
| Shift strategy | Vehicle-controller shift points and optional torque-derived calculations are separate from engine limits. [S1] | Raising redline may leave automatic/arcade shifting unchanged. Shift adjustments need their own option that respects capabilities. |

Here's the conversion: **angular velocity = RPM × π / 30**. Don't pass an RPM value straight into a rad/s setter.

### 3.2 Proposed user-facing controls

**Initial mode: limiter adjustment within the existing supported engine/curve envelope.** [L, P]

- Show stock limit, requested limit, acknowledged setting, limiter type, and observed RPM as separate values.
- Give users a slider, a numeric input, and a per-engine **Restore stock** action.
- Discover bounds from the engine and adapter. Don't hard-code one redline range for every gasoline, diesel, or modded engine.
- Don't silently remove thermal simulation, raise physical damage tolerance, or extend the torque curve when someone changes redline.
- Explain why a requested higher value isn't supported instead of showing a slider that does nothing.
- Keep damage/protection settings separate. Label them as protection changes, not guarantees of stability.
- Disable editing for unsupported devices, unknown limiter implementations, missing readback, a policy lock, or a vehicle that isn't locally owned.

**Later experimental mode: extend the usable RPM envelope.** [C, P]

- Choose a synthetic torque-curve extension policy up front. One example is a limited taper from a supported endpoint. Don't claim realistic output.
- Define how curve editing, the existing power multiplier, turbo/supercharger behavior, and stock restoration fit together.
- Show estimated output and warn that torque generation, friction, inertia, valvetrain behavior, drivetrain damage, and controller limits may still stop the engine from doing what you asked.
- Apply a hard allocation/point budget. A huge finite RPM can still create a huge integer-indexed torque table, so finite-number validation alone isn't enough.
- Treat transmission shift changes as a separate option. Never silently rewrite the user's gearbox or ESC behavior.

**Recommendation:** ship the limited, supported redline adapter first. Research synthetic curve extension as an experimental follow-up, not a hidden side effect.

### 3.3 Integration hazards to solve before building

1. **Protection conflicts:** existing engine protection changes `maxPhysicalAV` and restores captured engine fields. RPM changes must work with that restoration so a protection toggle doesn't accidentally undo them.
2. **Power-baseline conflicts:** existing power scaling uses saved `beamdeckBase` torque data. A new curve editor needs one clear base/transform pipeline. Otherwise the next power change can overwrite curve edits.
3. **Gear-controller conflicts:** ABM's added-gear feature also restores controller shift-point tables. Any future shift-RPM editor has to share that state instead of keeping competing snapshots.
4. **Temporary-limit ownership:** ESC and launch controllers can legitimately lower the current limit. Report their intervention instead of repeatedly undoing it.
5. **Multiple engines:** identify each device. Preflight all selected engines before you change any. An unsupported second engine must not leave a partly applied tune.
6. **Electric vehicles:** motor maximum speed is a different capability. Don't route every device through the combustion-engine adapter.
7. **Reset lifecycle:** define what a physics reset, respawn, vehicle switch, bridge reload, and policy cleanup each restore. Readback must detect that the previous instance no longer exists.

## 4. Requested feature: power without clicking Apply

### 4.1 Why the current path can't stay as it is

We did a read-only inspection of `src/Main.qml`, `src/backend.cpp`, and the vehicle bridge. Here's what we found:

- Power currently submits a combined payload with power, gravity, and beam protection.
- The backend returns while busy instead of keeping the newest power edit.
- There's one pending request, and unrelated catalog requests can occupy it too.
- Success handling emits an action-state change before the accepted tuning-state update finishes. A queue drained from that signal could read stale values.
- Successful tuning writes preferences synchronously. Persisting every slider sample would cause needless writes.
- The game resolves the active vehicle when it processes a request. UI-only cancellation isn't enough to stop a delayed request from reaching a different vehicle.

Qt's `Slider.live` updates the QML value. It does **not** implement network auto-apply. `onMoved` tells you the user interacted with the slider, while `onValueChanged` also runs for bindings and initialization. [S8]

### 4.2 Proposed interaction

- Dragging the power slider previews the value immediately and applies it through a coalescing backend queue.
- Starting tuning parameters to evaluate: **150-250 ms debounce**, with a separate send-rate ceiling around **four requests per second** if you want intermediate updates during a long drag. These are proposals, not measured optimal values.
- Releasing the slider flushes the latest valid value as soon as the current request allows.
- A trailing debounce alone waits until the user stops dragging, so it won't provide continuous live feedback. If you turn on continuous application, use a rate-limited latest-value scheduler during the drag plus a trailing final-value flush. Don't combine timers in a way that duplicates requests.
- Numeric input applies on Enter or when valid editing finishes. Empty text, partial exponents, NaN, infinity, and derived overflow must not trigger a command.
- A small state label shows **Pending → Applying → Accepted**, or the real error. Observed game values stay separate from accepted requests.
- A visible **Live power** switch may let users opt into staged editing, but the default workflow shouldn't require clicking the old combined Apply button.
- Beam protection and gravity keep their own controls. A power adjustment must never reset either one.

### 4.3 Required command architecture

Proposed flow:

```text
user edit
  → validate and preview
  → target-bound power intent
  → debounce/rate control
  → one in-flight request + one replaceable pending value
  → receiver policy and ownership checks
  → game/vehicle target-generation checks
  → fixed power action and result
  → atomically update accepted state
  → send latest pending value, if still valid
```

Required properties:

1. **Dedicated fixed action:** separate engine power from beam protection and world gravity. No arbitrary Lua execution, and no read-modify-write of unrelated draft values.
2. **Target identity:** bind each intent to game session, vehicle ID, vehicle-instance generation, and engine device selection. Vehicle IDs alone can be reused after respawn. The receiver and game must reject stale targets before any mutation.
3. **Latest value wins:** don't buffer every drag sample. At most one in-flight value and one replaceable pending value for the active target.
4. **Cancellation:** clear pending work on car/server/session change, disconnect, policy denial, reset/respawn, capability loss, or an invalid target generation. Never replay an old drag automatically on reconnect.
5. **Timeout semantics:** a timeout means the outcome may be unknown, not that nothing happened. Stop automatic replay, request readback, and require a fresh valid intent. Account for a paused vehicle VM and late acknowledgments.
6. **Ordered completion:** finish accepted-state updates before draining the next intent. A stale acknowledgment must not update the next car's displayed settings.
7. **Persistence:** save the final accepted setting after idle/release instead of synchronously for every intermediate sample. Keep reusable presets separate from transient runtime state.
8. **Backpressure and fairness:** catalog loading must not discard edits, and live power must not starve recovery, policy cleanup, or other commands. Cleanup has priority over all queued tuning.
9. **Policy checks at execution:** passing the guard when a slider moves doesn't authorize execution after a server change or lease expiry.
10. **Idempotent transforms:** keep non-compounding scaling and preflight every derived torque value for overflow.

This is a backend/bridge feature with a QML gesture source, not a QML timer bolted onto the existing combined Apply action.

## 5. Additional requested priorities: clutch, persistence, and gearbox UX

These are clear user requirements added after the initial research. The user reports repeated clutch failures, loss of grip/gearing/dimension tunes after resetting, and an unintuitive gearbox editor that doesn't load the car's current ratios. Those reports motivate the requirements below. They aren't a diagnosis of the clutch's failure mechanism. We haven't implemented anything or validated a new clutch API in this documentation update.

### 5.1 Indestructible clutch / clutch protection

**Requested outcome:** a separate clutch-protection toggle that prevents supported clutch failures while tuning and driving. [P]

- Investigate the actual installed clutch/torque-converter devices and controller behavior before picking an implementation. Separate thermal fade, permanent wear/damage, torque overload, slip, and unrelated gearbox/synchronizer failure.
- Discover support per device. A manual friction clutch, DCT clutch pack, automatic torque converter, and modded clutch aren't interchangeable implementations.
- Keep clutch protection independent of engine/beam protection. Engine protection doesn't prove the clutch is protected.
- Prefer a narrowly scoped supported damage/temperature/wear adapter over disabling all drivetrain simulation. Preserve normal engagement and torque transfer where you can, and document any thermal freeze or other simulation compromise.
- Show clutch temperature, slip, wear/damage, and protection status only when you have reliable readback. Disabling protection must restore original behavior and limits without silently repairing existing damage.
- Treat repairing an already damaged clutch as a separate recovery operation that may need a reset/rebuild. Don't advertise that enabling protection repairs it.
- Include accepted protection state in the reset-persistent recipe described below. Full reset removes the protection and restores the baseline.
- Until we verify the relevant mechanism, mark this as **clutch protection: adapter research required**, not a proven universal indestructibility API. Extreme torque can still destabilize or break other drivetrain components.

### 5.2 Keep tunes through ordinary vehicle resets

**Requested outcome:** an ordinary in-game reset repairs/resets the car but doesn't discard its accepted grip, gear ratios, dimensions, or other supported ABM tuning. [P]

Keep two things separate:

1. **Baseline:** the unmodified state of the currently loaded vehicle configuration, captured before ABM transforms and tied to compatible model/config/device metadata.
2. **Accepted tune recipe:** declarative, successfully accepted settings, including handling, ratios, dimensions, power/RPM, and applicable protection states. Draft text and unacknowledged commands aren't accepted tune state.

Proposed lifecycle:

```text
ordinary reset detected
  → invalidate old vehicle-generation commands and acknowledgments
  → wait for the reset/rebuild and supported devices to be ready
  → verify same compatible vehicle/configuration and ownership
  → check current server policy and capabilities
  → capture/verify a fresh unmodified baseline
  → reapply the accepted recipe once in adapter-defined dependency order
  → read back results and show Restored / Partial / Failed
```

Requirements and limits:

- Apply multipliers against a fresh baseline, never against the already tuned state. Repeated resets must not compound grip, gearing, or dimensions.
- For dimensions, wait until reset geometry and beam rest lengths are ready. Never capture a half-reset or previously stretched shape as the new baseline. Keep the existing experimental-physics warning.
- Rebuild gearbox ratios, dependent controller shift data, synchronizer/auxiliary tables, and protection adapters in a validated order. Don't replay a ratio list against the wrong transmission.
- Don't blindly preserve temporary effects such as fire, bouncing, burst tires, or a one-shot repair command. Persistence is for declared settings, not every historical action.
- Ordinary reset persistence is required for the same compatible car. Applying across another car, a changed parts configuration, a new server/session, or an application restart is a separate preset/persistence decision. Don't silently transfer tunes.
- Policy cleanup, ownership loss, incompatible configuration changes, and full reset must suspend or clear automatic restoration as appropriate. A reset must never re-enable a tune that cleanup just removed on a blocked server.
- A failed or partial restoration must show affected settings, cancel unsafe dependent steps, and offer recovery. Don't display the whole recipe as applied when only one part succeeded.
- Keep queued intent cancellation from section 4: a reset discards old commands. Any restoration is a **fresh**, validated request for the new vehicle generation, not a replay of stale packets.

### 5.3 Full reset at the bottom of the Car tab

Add a clearly separated **Reset all car tuning** button at the bottom of the Car tab, available no matter which workshop group is selected. Keep its purpose distinct from an ordinary in-game reset. [P]

- Restore the loaded configuration's pre-ABM baseline: power, RPM, clutch/engine/beam protection, handling, gear ratios/controller state, and experimental dimensions, plus future registered car-tuning adapters.
- Clear the accepted runtime recipe and pending edits **before** restoring/resetting. Advance the generation/revision so late replies or reset hooks can't silently turn the tune back on.
- Explain whether the operation needs a physics reset or respawn, whether existing damage is repaired, and whether the car must be stopped. Confirm disruptive behavior. Never silently reset a moving car just because someone pressed a normal UI button.
- If reverse transforms can't safely restore a setting, rebuild from the original loaded configuration without overwriting its source file. Unsupported or failed restoration must stay visible.
- Clear current runtime tuning, not saved named presets or unrelated user files. Don't replace a user's custom loaded configuration with an unrelated factory configuration.
- World settings such as gravity/time aren't car settings. List them separately and give users an option/action to restore ABM-owned world changes instead of silently altering the world from this button.
- Keep safe restoration reachable under a policy lock without granting permission to apply a new tune. Cleanup/restoration still needs ownership and target checks.

### 5.4 Gearbox editor that starts from the real car

**Requested outcome:** load the car's actual current gear data instead of a generic text list, give a clear overview, and make **+ Gear** suggest a useful ratio from previous gears. [P]

Readback and presentation:

- Read current ratios from the selected supported gearbox, including gear indexes, forward count, neutral, reverse, and gearbox/device identity. Refresh on vehicle/configuration change, reset, and successful application.
- Keep the difference between loaded baseline ratios, current observed ratios, and unsaved edits. Don't overwrite active user edits with periodic telemetry. Indicate changed readback and offer reload/rebase.
- Replace the comma-separated primary editor with a compact table: **Gear | Current ratio | Edited ratio | Gap / predicted RPM after shift**. Show reverse and neutral separately. Don't renumber them as forward gears. Keep an advanced copy/paste option if it's useful.
- Provide per-row editing, add/remove last forward gear, restore loaded ratios, and clear validation feedback. Highlight changed cells and disable unsupported actions.
- Show final drive separately when it's reliably available. It isn't an extra gearbox gear. Optional speed-at-RPM estimates need real tire/effective rolling-radius and final-drive data, must state assumptions, and aren't guaranteed top speed.
- Keep validating every dependent shift-point/synchronizer table. A readable editor must not bring back the added-gear controller crash. Unsupported automatic/DCT/CVT editing stays disabled until an adapter exists.

**+ Gear suggestion based on previous gears:**

1. Use the valid edited forward ratios, or the actual current ratios if there are no edits. Never start from unrelated demo/default values.
2. For a conventional descending positive sequence, calculate adjacent retention factors `q_i = ratio[i] / ratio[i-1]` over the last few forward steps. Each usable factor must be finite and between zero and one.
3. Propose `nextRatio = lastRatio × q`, where `q` is a stable recent progression estimate, such as the median of up to the last three usable factors. This is a proposed heuristic that needs validation, not a proven optimal algorithm.
4. Show the result as **Suggested** and keep it editable. For example, recent ratios `1.40, 1.00, 0.75` produce retention factors about `0.714` and `0.750`. Their median gives a suggested next ratio near `0.55`.
5. Explain predicted RPM retention: ignoring shift transients and slip, `RPM_after ≈ RPM_before × nextRatio / currentRatio`. Later, use supported torque/power-band data to warn about an excessive RPM drop or offer an acceleration/cruising target.
6. If only one usable forward ratio exists, ask for a desired gap/target or a manual ratio. If the sequence is unusual, increasing, invalid, or too sparse, explain why no reliable automatic suggestion exists instead of silently inventing an unrelated ratio.
7. Enforce ratio bounds, transport/table budgets, controller support, and numerical precision. If rounding/minimum limits would produce a duplicate or invalid added gear, disable automatic addition with a reason.

"Best ratio" can't mean one universal optimum. Acceleration, cruising RPM, engine power band, final drive, tires, drag, traction, and shift strategy all affect the result. The initial requirement is **a sensible, transparent continuation of the car's existing progression**, not an unexplained fixed multiplier. Performance-based optimization should come later as a separate feature.

## 6. Feature catalog for the larger update

Priority: **P0** foundation/requested; **P1** strong next additions; **P2** later or optional; **P3** research-heavy experiments.

Effort is relative engineering scope, not a delivery estimate: **S / M / L / XL**. Risk describes potential effects and compatibility burden, not a safety certification. Scope **owned MP** stays subject to ABM's guard and server rules. **SP** means single-player only by default.

| Priority | Proposed addition | Value / dependency | Effort; risk; scope | Evidence |
|---|---|---|---|---|
| P0 | Live, target-bound engine power | Requested workflow; needs a queue, readback, cancellation, and the separate action described above. | L; medium; SP / owned MP | [L, P], S8 |
| P0 | Supported maximum-RPM adjustment | Requested control; needs per-engine adapters and limiter/curve/damage separation. | L; high; SP / owned MP | [L, P], S1 |
| P0 | Clutch-specific protection | Requested because the clutch keeps failing; first identify device-specific thermal/wear/overload mechanisms and reversible protection. | L; high; supported owned vehicle | [P], adapter research required |
| P0 | Reset-persistent tuning and full-reset footer | Restore accepted grip, gears, dimensions, and other supported settings after ordinary resets; a separate bottom-of-Car full reset clears the recipe and restores baseline. | L; high lifecycle/physics risk; owned vehicle | [P], section 5 |
| P0 | Actual-ratio gearbox table and smart + Gear | Read current device data, display individual gears and RPM gaps, suggest the next ratio from existing progression with an editable explanation. | L; high controller compatibility risk; supported owned vehicle | [P], section 5.4 |
| P0 | Requested/accepted/observed values | Explain what changed, what was rejected, and what another controller overrides. | L; low mutation risk; all modes | [P] |
| P0 | Tune snapshot, Undo, and Restore stock | Shared restoration ownership for engine, gearbox, handling, dimensions, and future controls. Undo isn't a universal repair operation. | L; high restoration complexity; owned vehicle | [L, P] |
| P0 | Compatibility and disabled-control explanations | Show a missing API, unsupported controller, damaged tire, moving vehicle, policy block, or stale bridge instead of a generic disabled button. | M; low; all modes | [L, P] |
| P1 | Named presets and A/B comparison | Export/import limited declarative presets; validate capabilities, vehicle/config identity, schema, and policy before applying. Never import executable Lua. | L; medium; owned vehicle | [P], inspiration S3/S6 |
| P1 | Vehicle-native tuning-variable browser | Expose supported categories, units, ranges, descriptions, and steps. Separate staged rebuild settings from live controls. | L; medium/high; initially SP | [D, L, P], S2 |
| P1 | Torque/power graph with stock overlay | Graph supported engine data and multiplier effects. Label estimates; wheel power and real dyno results are different quantities. | M/L; low for read-only graph; all modes | [L, P], S1/S3 |
| P1 | Acceleration and braking tests | Configurable speed intervals, braking distance, reaction-free rolling starts, and local best/session deltas. Requires game-clock instrumentation. | L; medium measurement risk; initially SP | [C, P], S7 |
| P1 | Resource panel | Fuel, battery charge, nitrous, and air storage only when the specific storage device supports it. Don't assume all containers are interchangeable. | M; medium; owned vehicle | [L, P], S3 |
| P1 | World playground | Time of day, wind, and existing gravity in one coherent panel, with stock/captured restoration. Global changes stay SP by default. | M; medium; SP | [L, P], S3 |
| P1 | Slow-motion controls | Presets and a clear simulated-time display. Inspected `setTargetSpeed()` supports roughly 0.001 to 1; don't promise fast-forward through that API. | M; medium; SP | [L, P] |
| P1 | Telemetry recording and CSV export | Limited opt-in capture with units, clock, car/config/tune metadata, and a chosen save destination. No player-list collection by default. | L; low privacy/resource risk; policy-aware | [P], inspiration S6/S7 |
| P1 | Crash/command diagnostics bundle | Redacted bridge versions, capabilities, recent fixed-action errors, and restoration status; preview before export. | M; low privacy risk if redacted; all modes | [P] |
| P2 | Power-by-gear / progressive power map | Extend scalar power into a map. Needs transform composition, supported gear detection, and transient-state handling. | L; high; initially SP | [C, P], S3/S6 |
| P2 | Launch control / two-step integration | Prefer existing supported controller settings rather than overwriting temporary limiter ownership. | L; high; initially SP | [L, P], S3 |
| P2 | Shift-RPM and gearbox strategy | Extend the existing gearbox editor with supported automatic/arcade thresholds and optional torque-aware suggestions. No universal gearbox claim. | L; high; owned vehicle | [D, L, P], S1 |
| P2 | Tire pressure, alignment, and suspension tuning | Start with exposed vehicle variables. Grip coefficient isn't pressure; geometry and pressure can change spring behavior. | L; high; staged rebuild / initially SP | [D, L, P], S2 |
| P2 | Differential and final-drive workbench | Prefer exposed configuration variables or documented device adapters; validate gear ratios and controller assumptions. | L; high; initially SP | [D, C, P], S2/S3 |
| P2 | Paint and finish preview | Explore existing configuration paint setters, correct layer mapping, and save-as. Don't assume every mesh supports arbitrary paint layers. | M/L; medium; initially SP | [L, P] |
| P2 | Cruise / repeatable test target | Supported speed and acceleration targets for private testing, with a prominent disable action. Never represent it as autonomous safe driving. | M; high control risk; SP | [L, P] |
| P2 | Custom-config collections and provenance | Tag builds by purpose, record base model/mod dependencies and tune version, compare missing or changed content. Builds on existing garage filters. | M/L; low; local files | [P] |
| P2 | Preset challenges | Local braking, hill-climb, traction, and acceleration experiments using fixed tune/map conditions. No certified or anti-cheat leaderboard claim. | L; medium; SP | [P], S7 |
| P3 | Turbo/boost-by-RPM map | Requires a real compatible turbo API, units, transient limits, and thermal/damage handling. An advertised "safe boost" value isn't a guarantee. | XL; high; SP first | [C, P], S3/S6 |
| P3 | Full torque-curve editor | Limited points, interpolation, units, base-curve ownership, power-transform composition, and clear synthetic behavior. | XL; high; SP first | [C, L, P], S3 |
| P3 | Node mass, aero, and deeper body tuning | Requires inertia/physics consistency and recovery. Extend current experimental dimensions only after understanding their failure modes. | XL; very high; SP first | [C, L, P], S4 |
| P3 | Replay bookmarks / ghost comparison | Public replay functionality exists, but an ABM control API and compatible ghost implementation haven't been established here. | XL; unknown; SP | [P]; follow-up lead only |
| P3 | Organizer-controlled multiplayer integration | Separate server plugin with clear permissions and schema/rate validation. No client-only synchronization or policy-bypass claim. | XL; high trust/sync risk; opt-in servers | [D, P], S9 |
| P3 | Optional engine-simulation adapters | Integrate compatible installed mods for AFR, wear, temperature, or starter systems without replacing engine core logic automatically. | XL; high compatibility risk; opt-in | [C, P], S5 |

### Highest-value original reasoning

- **A preset without a manifest is unreliable.** Store model/config identity, engine/controller identifiers, game/bridge version, required capabilities, units, and whether a rebuild is required. Warn or refuse when those assumptions change.
- **A successful request is not proof of a stable tune.** Record that the command was accepted, then show observed values and any instability symptoms separately. Don't label numerical acceptance as a safety check.
- **A reset button is not necessarily Undo.** Some changes need baseline restoration before a reset; others need respawn. Recovery should say which operation will happen and whether it can interrupt driving.
- **More raw fields produce less practical customization.** A searchable, capability-aware native-variable panel with explanations is more useful than an unrestricted engine-table editor.
- **Measurements make customization meaningful.** A/B presets, repeatable tests, and graphs give you a reason to use power, grip, gearing, and RPM controls together.
- **Compatibility is a feature.** Game updates, modded controllers, stale vehicle extensions, and platform packaging deserve a visible health panel, not hidden best-effort behavior.

## 7. Important implementation leads and limits

These are future implementation leads, not promises of runtime support.

### Native configuration variables

The public variable specification provides names, units, categories, descriptions, numeric bounds, and display steps. Display limits aren't necessarily actual validation limits. Honor `hideInUI` and keep metadata separate from safe runtime mutability. [S2]

Local `lua/ge/extensions/core/vehicle/partmgmt.lua` exposes `setConfigVars()` and configuration accessors. Its merge path normally respawns the vehicle, and the source says a rebuild is generally required. So:

- Discover only variables exposed by the selected vehicle/configuration.
- Stage edits with an **Apply and rebuild** action while stopped.
- Confirm disruptive rebuilds. Preserve the original config and offer save-as separately.
- Don't use the live power queue for suspension, alignment, or arbitrary JBeam changes.
- Treat actual multiplayer configuration propagation as a separate research/validation task.

### Existing source leads worth adapting

| Area | Installed-source lead | Limitation |
|---|---|---|
| World speed | `lua/ge/simTimeAuthority.lua`: `setTargetSpeed()` | Observed clamp is 0.001 to 1; SP-only proposal. |
| Environment | `lua/ge/extensions/core/environment.lua`: time-of-day and wind setters | World effects are global. Owning a car doesn't authorize global MP changes. |
| Resources | Vehicle `energyStorage` device implementations expose `setRemainingRatio()` | Device-specific behavior and dependent controllers need validation. |
| Cruise | `lua/vehicle/extensions/cruiseControl.lua`: speed, enabled, target-acceleration setters | Safety and cancellation must be tested; SP-only proposal. |
| Paint | Vehicle part-management configuration paint setter | Layer interpretation, persistence, and any rebuild/network behavior need dedicated investigation. |
| Mass | `obj:setNodeMass()` appears in vehicle code | A node assignment alone isn't a complete valid mass/inertia update. |

## 8. UX plan: more depth without another crowded page

Keep the existing compact, square, dark **Qt Basic / Omarchy** look. No Material redesign.

- **Engine & protection:** live power, supported RPM, output graph, protection. Advanced curve/boost controls stay collapsed and marked experimental.
- **Handling & gearbox:** existing grip/brakes/steering and gears, then supported native variables and shift strategy. Visually separate live controls from rebuild-only settings.
- **World & recovery:** SP environment/slow motion, resources where appropriate, recovery with clear reset/respawn consequences. Emergency stop/recovery must stay reachable without hunting through advanced forms.
- **Body:** paints and native body variables before expanding experimental geometry/mass features.
- **Presets and measurements:** a compact reusable drawer or dedicated workspace once scope warrants it, not another dropdown entry for every small feature.
- **Shared Car footer:** a clearly separated **Reset all car tuning** action below the workshop content, accessible from every group. Ordinary in-game reset keeps the accepted tune; this action clears it.

Every editable row should show units, stock/current/requested state where available, a reset affordance, and a reason when it's unavailable. Don't show a control as usable just because a vehicle exists. Avoid "indestructible" or "safe" promises beyond the actual supported mechanism.

## 9. Multiplayer and safety rules for every proposal

1. Preserve the current exact-name official-server whitelist and fail-closed classification behavior. This research doesn't change it.
2. Reject queued and newly arriving mutations if policy becomes blocked. Stop reapplication and prioritize existing cleanup.
3. Bind commands to the local owned vehicle instance. Never let a preset select another player's vehicle.
4. Keep global world/time/weather/AI changes SP-only unless a separately designed server integration authorizes them.
5. Don't imply that local dimensions, power, or other runtime changes are correctly synchronized to peers. Each feature needs its own synchronization study.
6. A mod-reference `TriggerClientEvent()` is a local client event, not a broadcast-to-all-players API. A server plugin needs validated event payloads and real server-side permission logic. [S9]
7. Keep the documented limits of the client guard: it's cooperative behavior, not tamper-proof anti-cheat, organizer approval, or protection from staff consequences.
8. Don't adopt community instructions for bypassing multiplayer restrictions, anti-cheat, camera enforcement, or event rules.
9. Don't edit career progression, money, unrelated user files, or installed third-party mods as an implicit side effect.

## 10. Suggested delivery waves

### Wave A: requested controls and trustworthy state

Target-bound command coordinator, readback, independent live power action, supported RPM and clutch-protection adapters, reset-persistent accepted recipes, the full-reset footer, actual-ratio gearbox table, progression-based + Gear suggestions, capability explanations, and protection/gear-state composition.

**Exit criterion:** power changes need no Apply click, never change beam/gravity settings, and never reach a stale car; RPM/clutch controls describe their supported behavior accurately; ordinary resets keep accepted grip/gearing/dimensions; full reset clears them; the gearbox starts from actual data and explains every suggested added ratio.

**Critical scope correction:** this is a milestone, not one implementation batch. Start with slice A0 in section 13. Don't attempt the entire milestone in a single build/review pass, and don't let speculative adapters delay the state/recovery foundation.

### Wave B: useful customization

Presets with compatibility manifests, A/B comparison, native tuning-variable browser, staged rebuild workflow, and stock/modified torque graphs.

**Exit criterion:** users can reproduce and reverse a tune without overwriting their original configuration or guessing which settings were applied.

### Wave C: experimentation tools

Performance timing, limited telemetry export, resources, SP environment/slow motion, and repeatable challenges.

**Exit criterion:** tests explain their clock and conditions, invalidate interrupted runs, and avoid claims of certified measurement precision.

### Wave D: optional high-risk adapters

Synthetic RPM/torque extensions, boost maps, advanced drivetrain/physics work, optional mod adapters, and organizer server integration. Each needs its own research and go/no-go review. None is required to deliver Waves A through C.

## 11. Future acceptance checklist (not executed in this task)

### Live power

- Mouse, wheel, touch, and keyboard edits coalesce. Initialization and telemetry updates don't send commands.
- The final slider/text value isn't lost while another request is busy.
- Beam protection and gravity stay unchanged.
- Delayed, duplicate, reordered, and timed-out replies can't update a different car's settings.
- Switching cars, respawning with a reused ID, changing servers, pausing, disconnecting, and losing policy permission cancel pending work correctly.
- Invalid/overflow inputs cause no mutation. Multi-engine preflight prevents partial changes.
- Preference writes are coalesced, not performed for every drag sample.

### Maximum RPM

- Test supported `soft`, `timeBased`, and `rpmDrop` configurations, plus variants we don't support.
- Test raised and lowered limits, insufficient torque-curve coverage, stock restoration, and mechanical over-rev separately.
- Verify behavior with ESC, two-step, engine protection, engine-strength scaling, and ABM gearbox modifications.
- Test multiple engines, modded controllers, missing fields, reset/respawn, and policy cleanup.
- Enforce finite values, coherent derived fields, limited curve allocation, and a clear synthetic-curve warning if that mode is ever added.

### Larger workbench

- Round-trip declarative presets without executable content or a silent fallback for unsupported settings.
- Confirm native tuning variables respect metadata and use rebuild flows the user triggers.
- Measure timing with an identified game clock. Label slow motion, sampling/interpolation limits, and invalidated runs. Desktop telemetry alone isn't certified drag-strip timing.
- Verify Windows, AppImage/portable Linux, and Arch packaging for the new version. Previous-release CI isn't proof that future changes work.
- Photograph every changed page, menu, disabled state, and narrow-window layout using fake demo data.

### Clutch, reset persistence, and gearbox editor

- Identify and test the actual supported clutch failure mechanisms. Protection preserves intended engagement, disabling it restores behavior, and unsupported devices show a reason rather than false protection.
- Ordinary resets keep accepted grip, gear ratios, dimensions, power/RPM, and protection without accumulating multipliers, duplicate gears, or geometry scaling.
- Delayed callbacks, repeated resets, reset during an in-flight change, respawned/reused vehicle IDs, configuration changes, and modded devices can't restore the wrong recipe.
- Policy cleanup, server transitions, and ownership loss prevent automatic reapplication. A full reset clears the recipe before any reset callback can restore it.
- The full-reset button appears at the bottom of every Car group and restores the loaded configuration's baseline without deleting named presets, editing source configs, or silently resetting world settings.
- Actual ratio readback covers signed indexes, neutral/reverse separation, current edits, refresh after reset, and unsupported transmissions. External ratio changes don't silently overwrite unsaved edits.
- Test + Gear with stock progressions, edited ratios, sparse data, unusual sequences, minimum/maximum ratios, precision limits, and payload budgets. Suggestions are editable and explain RPM retention. We don't promise a universally optimal ratio.
- Extended and reduced gear sets keep valid controller/synchronizer tables through apply, reset restoration, failure rollback, and full reset.

## 12. Decisions to settle before implementation

- First RPM release: supported limiter adjustment only, or experimental torque-curve extension too? **Recommendation: limiter-only first.**
- Live-power default and rate: always live in the current supported envelope, or a visible live/staged preference? **Recommendation: live by default, conservative coalescing, final-value guarantee.**
- Reset persistence is now a requirement for the same compatible car. The open question is whether it should also cover app/game restarts or different configurations. **Recommendation: keep accepted settings through ordinary resets, use named presets for broader reuse, and never silently transfer tunes across cars.**
- Unsafe/rebuild settings: which need stopped state, confirmation, or SP-only scope? **Recommendation: establish an action classification before adding the variable browser.**
- Which modded engines/controllers get maintained adapters? **Recommendation: vanilla capability adapters first, with clear unsupported reasons elsewhere.**
- Is organizer integration a separate project? **Recommendation: yes. Don't couple the first live-power/RPM release to it.**

## 13. Critical review: not yet a build-ready massive update

**Verdict:** the document can guide a limited foundational implementation. It isn't a complete specification for one massive release. Right now it holds both verified leads and unresolved experiments. More feature ideas aren't the bottleneck. State ownership, reset semantics, and demonstrable recovery are.

### 13.1 Hard blockers and specification gaps

| Critical issue | Why it matters | Required decision or proof |
|---|---|---|
| Wave A is too broad | Live commands, lifecycle persistence, geometry, clutch physics, RPM, and gearbox controllers are separate high-risk changes. Debug them together and you obscure what broke the car. | Split into independently reviewable, versioned slices. Keep all requested outcomes as milestone requirements. |
| Clutch protection is still unproven | The reported failure could be heat, wear, overload, a synchronizer, or a modded controller. Disabling one mechanism may not fix it and may change engagement physics. | Identify the actual failing device and damage path on a reproducible supported vehicle before promising protection. Missing evidence means unsupported, not a toggle that looks enabled. |
| Reset callbacks aren't a complete lifecycle contract | Reset, repair, recovery teleport, respawn, parts replacement, mission change, and policy cleanup need different behavior. Vehicle IDs can survive or be reused. | Define clear transition events and generation handling, with a tested source of reset completion/readiness. Don't rely on an arbitrary delay or a single generic reset hook. |
| Persistent dimensions can create a failure loop | An unstable tune could be reapplied after every repair and break the car again right away. A reset may start while moving or before clean geometry is available. | Keep the accepted recipe but suspend unsafe restoration when necessary. Require a valid baseline/readiness and the existing stopped-state rules. Add a retry limit and an escape hatch that works without a successful tuning round trip. |
| "Stock" and "full reset" are ambiguous | Factory defaults, a loaded custom config, and a snapshot already affected by another mod are different baselines. | Define baseline as the loaded configuration before ABM transforms, never unrelated factory values. If you can't reconstruct it, say so and offer a reload. Never claim verified stock restoration from a damaged snapshot. |
| No complete transaction contract | Preflight prevents some failures, but an exception can still happen halfway through applying multiple devices or a recipe. Not every physics change is safely reversible. | Report per-adapter outcomes. Declare which operations support rollback and which need a stopped rebuild. Never claim atomicity or success for a partly applied recipe. |
| Competing transforms and mods | Power, curve edits, RPM, protection, gearbox state, and another mod can write the same fields or wrap the same methods. | Use one ABM baseline/transform owner. Detect supported external changes. Don't repeatedly overwrite another controller or restore stale function pointers over its newer changes. |
| Queue freshness is insufficiently specified | Cancellation in the UI doesn't remove commands already queued in the game or vehicle VM. | Include target generation, monotonic intent revision, request identity, and expiry in the fixed-action contract. Recheck at the final mutation point. Distinguish superseded, rejected, accepted, and outcome-unknown. |
| Game pause and app failure are still edge cases | A timeout, application exit/crash, or receiver restart can leave you unsure which changes are active and which callbacks will still run. | Define receiver/bridge reconnection behavior and game-side cleanup/restoration responsibility. Don't depend on the desktop app staying alive to enforce policy or stop stale mutations. |
| "Best ratio" sounds more scientific than the data supports | A progression continuation isn't an optimized transmission. Torque curve, final drive, road load, traction, and shift behavior matter. | Label the initial result "Suggested next ratio," show the method and predicted RPM gap, and keep manual editing. Optimization needs a separately defined objective and more data. |
| Boolean capabilities are too shallow | "Has gears" doesn't prove editable tables, readback, restoration, reset persistence, or support for every action. | Define per-action capability details: supported device IDs, adapter/version, bounds/budgets, readback quality, reset behavior, and a machine-readable refusal reason. |
| Protocol compatibility isn't planned in detail | New target identity, readback, commands, and recipes can't safely depend on old bridges silently accepting unfamiliar data. | Version and negotiate the new contract. Old or mixed GE/vehicle components must disable new mutations with a clear upgrade reason. Don't reuse the current version fence as if the semantics were unchanged. |

### 13.2 Required reset/restoration state model

Specify at least these conceptual states before implementation:

```text
Ready → Applying → Accepted
Accepted → Ordinary reset → Waiting for compatible fresh instance
         → Restoring → Verified restored / Suspended / Partial failure
Any state → Full tuning reset or policy cleanup
          → Cancel intents + suppress automatic restoration
          → Restore/rebuild baseline → Verified baseline / Recovery required
```

- Full reset and policy cleanup outrank pending tuning and ordinary-reset restoration.
- The user's accepted recipe and what's currently verified in the game are separate records. A suspended recipe isn't an applied tune.
- Persistence must never mean forcing geometry, gear tables, or damage hooks every frame.
- Full reset must stay accessible when an adapter is broken or a request is stuck. Stopping restoration should be immediate. A disruptive physics reset/reload still needs confirmation and target checks.
- Reset persistence is required, but missing capability/readiness or a policy block must produce a visible suspension, not unsafe automatic application. Once things are compatible and authorized again, make the restoration/retry decision clear instead of replaying old network packets.
- Automatic restoration must not refuel, refill nitrous, replenish batteries, or erase unrelated damage as a side effect of restoring tuning. Define those recovery effects separately.

### 13.3 First implementation slice: A0

**Build this foundation first, after we approve implementation:**

1. Specify the versioned target/readback/action/result contract and reset lifecycle. Introduce a central accepted-recipe/restoration owner.
2. Read actual engine and gearbox state without mutation, including stable device identity and unsupported reasons. Replace placeholder gearbox data with a clear per-gear overview.
3. Implement independent live power using the rate-limited latest-value scheduler and execution-time target/policy validation. Leave unrelated drafts untouched.
4. Establish full-reset cancellation/suppression and baseline restoration for the tuning adapters we already support. Add the bottom-of-Car reset UI and show real restoration results.

**Follow with A1:** tested ordinary-reset persistence for grip and gearing, the explained + Gear suggestion, and dimension restoration only after we've proven its clean-baseline/readiness and failure-loop cases. Dimension persistence is still required. Staging it separately doesn't drop the request.

**Follow with A2:** supported maximum RPM and clutch protection, each backed by a concrete compatible vehicle/device test and a reversible adapter. These stay requested priorities, but don't present them as supported until we've established the relevant mechanisms.

Don't include torque-curve synthesis, turbo maps, mass/aero, generalized mod support, or the full native-variable browser in A0. They multiply compatibility risk without proving the foundation.

### 13.4 Evidence needed before calling a slice complete

- A small compatibility matrix: ordinary manual combustion car, a second materially different engine/gearbox, automatic/DCT refusal or supported behavior, EV refusal or separate support, modded-controller refusal, and multiple-device cases where relevant. Passing on one car isn't universal support.
- Deterministic request/reset tests covering delayed execution, partial failure, reused vehicle IDs, blocked policy during restoration, stale acknowledgments, and app/receiver reconnects.
- Real in-game validation of each mutation and its restore path. Mock Lua tests and screenshots can't establish that a clutch survives, geometry stays stable, or a vehicle reset actually completed.
- A clean final test result for the shipping commit. A failing test you fixed but never reran isn't verified release evidence. If the agreed one-review workflow prevents another validation pass, label the candidate unverified and delay shipping rather than claim it passed.
- A measured live-control latency/request-rate check under normal driving and pause/resume. Don't invent a responsiveness or no-FPS-impact claim from debounce settings.
- A documented recovery path for an unstable persistent tune, including how to suspend restoration before the next ordinary reset.
- Platform CI for the new commit and both protocol-compatible and known-incompatible installed bridge combinations. A successful package from the previous release doesn't prove the changed contract works.

**Go/no-go:** we're ready for A0 planning and implementation once authorized. We're not ready to promise the whole roadmap, universal clutch indestructibility, universally optimal gears, or safe persistent resizing across every vehicle. No app code was changed to produce this critique.

## 14. Source register

All sources below were accessed on **2026-11-10**. Community feature lists are author claims. We didn't install or validate their mods. Check documentation labels and paths against the game version when implementation starts.

| ID | Source | Research use |
|---|---|---|
| S1 | [BeamNG: vehicleController](https://documentation.beamng.com/modding/vehicle/vehicle_system/controller/main/vehiclecontroller/) | Official work-in-progress controller documentation: shift thresholds, torque-derived shifting, speed limits, torque-data reporting. |
| S2 | [BeamNG: JBeam variables](https://documentation.beamng.com/modding/vehicle/sections/variables/) | Official vehicle-native tuning-variable metadata, bounds, display properties, categories, and descriptions. |
| S3 | [Radial Cheat Menu](https://www.beamng.com/resources/radial-cheat-menu.37823/) | Experimental community feature breadth: engine/RPM/curves, handling, resources, environment, presets. Inspiration only; bypass instructions are excluded. |
| S4 | [Vehicle Global Strength, Weight, Size and Other Values Edit](https://www.beamng.com/resources/vehicle-global-strength-weight-size-and-other-values-edit-works-for-mods.14868/) | Community customization breadth and clear instability/restore/conflict warnings; not a drop-in runtime API. |
| S5 | [Advanced Engine Tuning and Wear](https://www.beamng.com/resources/advanced-engine-tuning-and-wear.37842/) | Community ECU/AFR/wear/starter feature inspiration and custom-engine compatibility concerns. |
| S6 | [FuelTech Boost Controller Dashboard](https://www.beamng.com/resources/fueltech-boost-controller-dashboard.37358/) | Community boost-map, telemetry, preset, and thermal-cut UI inspiration. Advertised safety/output claims aren't guarantees. |
| S7 | [Performance Timer Pack](https://www.beamng.com/resources/performance-timer-pack-drag-accl-braking.37236/) | Community acceleration, braking, drag timing, session comparison, and measurement-method inspiration. |
| S8 | [Qt Quick Controls: Slider](https://doc.qt.io/qt-6/qml-qtquick-controls-slider.html) | Official `live` and user-interaction `moved` behavior. Current documentation is newer than ABM's Qt 6.8 baseline; the cited interaction facilities predate it. |
| S9 | [BeamMP client mod reference](https://docs.beammp.com/scripting/mod-reference/) | Official client/server event interface and local-event distinction; informs future server-plugin design. |

Local source anchors, inspected read-only: `lua/vehicle/powertrain/combustionEngine.lua`; vehicle ESC and two-step controllers; the main vehicle controller and manual shift logic; `lua/ge/extensions/core/vehicle/partmgmt.lua`; ABM's current QML/backend/bridge; and earlier source-based environment, resource-storage, and cruise-control research in `sandbox-research.md`. No proprietary source excerpts are included.

**Bottom line:** build live power, maximum RPM, clutch protection, reset-persistent tunes, and an actual-data gearbox editor on a target-bound, recoverable state model first. Put a full tuning reset at the bottom of the Car tab. Then add native vehicle tuning, presets, and measurements. Keep deep simulation edits and multiplayer integration clearly scoped, optional, and evidence-driven.
