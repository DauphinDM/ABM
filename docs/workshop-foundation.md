# Workshop foundation: A0

Development version: 0.9.0. This implements the first foundation slice of [the research roadmap](massive-update-research.md), not all proposed features.

> This page records the 0.9 foundation. Development 0.10 adds [RPM editing, clutch protection and ordinary-reset recipes](reset-persistence.md); the deferred-feature statements below describe 0.9, not the newer implementation.

## Live power

Dragging the power slider or finishing a valid percentage edit requests a **power-only** change. There is no power Apply step. Beam protection and world gravity are not included in this action.

The backend owns scheduling: one in-flight request, one replaceable latest value, at least 250 ms between sends. Queued edits expire after three seconds. Invalid input, lost permission, disconnection, changed session, changed selected vehicle or changed vehicle-VM generation cancels queued intent. Failed/timed-out power requests are not replayed. Acknowledged preferences are saved after an idle interval rather than on every drag.

Vehicle readback distinguishes the latest ABM-accepted torque scale from a draft. It is not measured horsepower or proof that another mod has not subsequently modified the curve. Unknown scale is shown as unknown, not inferred as stock.

## Readback and target contract

Bridge and vehicle extension 0.9.0 are required. Readback includes a vehicle-VM instance/reset generation, supported engine maximum/limiter RPM values, and signed manual-gear indices/ratios. Engine discovery is limited to eight entries; manual readback supports indices -16 through 256. Huge/unsupported gearboxes must not be mistaken for a stock sample. This does not add RPM editing yet.

Power, full reset and the 0.9.0 tuning/handling/gears/engine/dimensions/stability adapter route carry the selected session, vehicle ID, generation, client revision and a short expiry. The receiver validates the current target; the GE bridge validates ownership, permission and fresh readback; the vehicle VM validates its own generation, expiry and increasing per-client revisions before execution. Reset changes the generation. Full reset clears the GE legacy reapply state before issuing the VM reset.

This is cooperative client protection, not anti-cheat. Legacy one-shot recovery/fun actions and garage operations have not been migrated into a universal transaction system. Older supported bridges retain their previous behavior; new live-power/full-reset actions require 0.9.0. Official-server name policy is unchanged.

## Gear editor

Rows show actual ratios beside edited forward ratios. Reverse and neutral are read-only. Incoming telemetry does not overwrite a dirty draft; switching vehicle generation reloads actual data. **Read actual** discards the draft. **Remove last** removes the final forward draft row.

**+ Gear** uses the median of up to three recent valid descending retention factors. A sequence of 1.4, 1.0, 0.75 suggests approximately 0.55. No valid descending pair means no automatic suggestion. Suggestions remain editable; torque curves, drag, tire radius and final drive are not being optimized. Applying still requires a stopped, supported manual gearbox and the existing payload/ratio bounds. Automatic/DCT/CVT support is not implied.

## Full reset

The button appears at the bottom of every Car group. Confirmation explains that this can repair/reset physics. The car must be stopped. Pending live power is canceled before reset dispatch; late power replies do not resurrect its draft.

The VM restores the existing ABM power/beam/handling, engine-hook and gearbox adapters and requests a physics reset. It does not delete loaded custom configs or saved files, and it does not change world gravity. A reply confirms adapter execution and a reset **request**, not completion of a clean physics frame. Adapter failures are reported and can leave partial restoration; no atomic physics rollback is claimed. Other mods' changes and unusual vehicles still need compatibility validation.

Ordinary-reset persistence, an indestructible-clutch adapter, editable RPM and complete recovery/preset transactions remain A1/A2 work. Do not treat this foundation as those features being complete.

## Native inspection blocker

The user-requested BeamNG MCP is saved globally in OpenCode as `https://127.0.0.1:29292/mcp`. During this work its TLS handshake failed; HTTP initialization also timed out. Certificate verification was not disabled. Installed game Lua and isolated mocks remain the evidence until the native endpoint is available. No live game mutation is implied by the configuration step.

## Development validation

One local validation/review pass: 31 Qt tests passed, including latest-value coalescing, cancellation on generation change, reset dispatch replacing pending power and bounded readback. The QML interaction test failed because the new confirmation dialog had an implicit-height binding loop; the standalone lifecycle test reported the same loop. The final fix assigns an explicit dialog content height. No second test pass was run after final fixes, following the requested build/review/stop workflow; the complete suite is therefore **not confirmed green**.

Lua bridge/VM, Node plugin/filter and headless CLI suites passed in that pass. Three demo captures (`engine`, `gears`, `blocked`) under `/tmp/opencode/abm-09-audit` were inspected. They show actual-versus-edited gear rows and distinct power readback; smaller windows scroll to the reset button. Logs also contain the previously identified corrupt installed T-series preview warning. Final fixes add a vehicle-VM stopped check immediately before full reset, cancel queued edits on invalid input, and refuse a ratio editor without actual readback. Live effects, reset completion, platform CI and the post-fix UI remain unverified.
