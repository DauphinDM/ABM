# Workshop foundation: A0

Development version: 0.9.0. This page documents the first foundation slice of [the research roadmap](massive-update-research.md), not every proposed feature.

> This page records the 0.9 foundation. Development 0.10 adds [RPM editing, clutch protection, and ordinary-reset recipes](reset-persistence.md). The deferred-feature notes below describe 0.9, not the newer implementation.

## Live power

Dragging the power slider, or finishing a valid percentage edit, requests a **power-only** change. There is no power Apply step. Beam protection and world gravity are not part of this action.

The backend handles scheduling. It keeps one request in flight and one replaceable latest value, with at least 250 ms between sends. Queued edits expire after three seconds. Invalid input, lost permission, disconnection, a changed session, a changed selected vehicle, or a changed vehicle-VM generation cancels queued intent. Failed or timed-out power requests are not replayed. Accepted preferences are saved after an idle interval, not on every drag.

Vehicle readback separates the latest ABM-accepted torque scale from a draft. It is not measured horsepower, and it doesn't prove that another mod hasn't changed the curve since. An unknown scale shows as unknown, not as stock.

## Readback and target contract

Bridge and vehicle extension 0.9.0 are required. Readback includes a vehicle-VM instance and reset generation, supported engine maximum and limiter RPM values, and signed manual-gear indices and ratios. Engine discovery is limited to eight entries. Manual readback supports indices −16 through 256. Huge or unsupported gearboxes must not be mistaken for a stock sample. This page doesn't add RPM editing yet.

Power, full reset, and the 0.9.0 tuning, handling, gears, engine, dimensions, and stability adapter route all carry the selected session, vehicle ID, generation, client revision, and a short expiry. The receiver validates the current target. The GE bridge validates ownership, permission, and fresh readback. The vehicle VM validates its own generation, expiry, and increasing per-client revisions before running anything. A reset changes the generation. Full reset clears the GE legacy reapply state before sending the VM reset.

This is cooperative client protection, not anti-cheat. Legacy one-shot recovery and fun actions plus garage operations haven't moved into a universal transaction system. Older supported bridges keep their previous behavior. New live-power and full-reset actions need 0.9.0. The official-server name policy is unchanged.

## Gear editor

Rows show actual ratios next to edited forward ratios. Reverse and neutral are read-only. Incoming telemetry doesn't overwrite a dirty draft. Switching vehicle generation reloads actual data. **Read actual** discards the draft. **Remove last** removes the final forward draft row.

**+ Gear** uses the median of up to three recent valid descending retention factors. A sequence of 1.4, 1.0, 0.75 suggests about 0.55. If there's no valid descending pair, ABM makes no automatic suggestion. Suggestions stay editable. ABM isn't optimizing torque curves, drag, tire radius, or final drive. Applying still requires a stopped, supported manual gearbox and the existing payload and ratio limits. Automatic, DCT, and CVT support is not implied.

## Full reset

The button appears at the bottom of every Car group. The confirmation explains that this can repair or reset physics. The car must be stopped. Pending live power is canceled before the reset is sent, and late power replies don't bring its draft back.

The VM restores the existing ABM power, beam, handling, engine-hook, and gearbox adapters, then requests a physics reset. It doesn't delete loaded custom configs or saved files, and it doesn't change world gravity. A reply confirms that the adapter ran and a reset was **requested**, not that a clean physics frame finished. Adapter failures are reported and can leave partial restoration. ABM doesn't claim atomic physics rollback. Other mods' changes and unusual vehicles still need compatibility testing.

Ordinary-reset persistence, an indestructible-clutch adapter, editable RPM, and complete recovery/preset transactions remain A1 and A2 work. Don't treat this foundation as those features being complete.

## Native inspection blocker

The user-requested BeamNG MCP is saved globally in OpenCode as `https://127.0.0.1:29292/mcp`. During this work, its TLS handshake failed, and HTTP initialization timed out. Certificate verification wasn't disabled. Installed game Lua and isolated mocks remain the evidence until the native endpoint is available. The configuration step doesn't imply any live game mutation.

## Development validation

One local validation and review pass ran: 31 Qt tests passed, including latest-value coalescing, cancellation on generation change, reset dispatch replacing pending power, and capped readback. The QML interaction test failed because the new confirmation dialog had an implicit-height binding loop. The standalone lifecycle test reported the same loop. The final fix assigns an explicit dialog content height. No second test pass ran after the final fixes, following the requested build, review, and stop workflow. The full suite is therefore **not confirmed green**.

The Lua bridge/VM, Node plugin/filter, and headless CLI suites passed in that same pass. Three demo captures (`engine`, `gears`, `blocked`) under `/tmp/opencode/abm-09-audit` were inspected. They show actual and edited gear rows and distinct power readback. Smaller windows scroll to the reset button. The logs also contain the previously identified corrupt installed T-series preview warning. Final fixes add a vehicle-VM stopped check just before full reset, cancel queued edits on invalid input, and refuse a ratio editor without actual readback. Live effects, reset completion, platform CI, and the post-fix UI remain unverified.
