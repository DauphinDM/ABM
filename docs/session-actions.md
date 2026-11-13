# Session action review

## API contracts we checked

- [BeamMP client-side reference](https://docs.beammp.com/API%20documentation/Client-Side/): `extensions.MPCoreNetwork.getCurrentServer()` returns the current endpoint and name. `extensions.MPVehicleGE.getPlayers()` is keyed by session player ID. `isOwn()` takes a **game vehicle ID**, not a server vehicle string. The bridge now resolves modules the same way for snapshots and authorization, preferring documented extension modules and keeping old global aliases as a fallback.
- [BeamMP server reference](https://docs.beammp.com/scripting/server/latest-server-reference/): player identifiers are a server-side API. Session player IDs are not permanent account IDs. The public forum username JSON lookup is separate, and it doesn't verify that the person in the game owns that forum account. No client account-ID or authentication API is invented here. Missing IDs don't imply guest status.
- [BeamNG virtual machines](https://documentation.beamng.com/modding/programming/virtualmachines/): GE and vehicle Lua state are separate. Roster and selection stay in GE. Resetting ABM trip statistics stays entirely in the local receiver, not the vehicle VM.

## Defects we addressed

Some ownership and command paths used only legacy BeamMP globals, while snapshots already supported extension modules. With only the documented namespace, owned vehicles could disappear from the selectable list. These paths now share module resolution and still refuse when multiplayer ownership is unavailable. Roster conversion also ignored the documented player-map key and inferred guest status from missing embedded IDs. Those two ideas are now kept separate.

Profile state could outlive invalid telemetry snapshots, and pending lookups weren't canceled immediately when the server or a same-ID player's name changed. The backend now updates the profile roster before notifying QML, includes the server identity in lookup context, and clears it on disconnect or invalid snapshots. Forum lookup results are size-limited, exact-name checked, explicit-intent only, and never followed across redirects. Player action feedback also shows on the card.

Sample players no longer open unrelated real forum pages. The owned-vehicle dialog explains why selection is disabled. Native selection API errors are reported, and a successful dispatch is described as a request, not verified game acceptance.

## Verification boundaries

The initial isolated test did **not** reproduce universal click failure. Basic copy and dialog buttons worked. It did expose an unrelated missing-engine-array QML binding error, which is now guarded. Regression coverage exercises real QML presses held across changing receiver telemetry, actual clipboard contents, browser URL dispatch intercepted locally, local trip reset, and confirmed selection dispatch. Forum success, malformed or oversized or mismatched or redirected responses, and late-copy cancellation use a fake network manager. They never use real usernames or forum traffic. Lua mocks cover extension-only modules, player-map keys including ID zero, ownership refusal, unverified-policy refusal, and native selection exceptions.

No live game session, real browser launch, package installation, or user game file was changed. Live BeamNG/BeamMP behavior still needs checking after you update and load the new bridge. Passing isolated tests doesn't mean every server or mod accepts vehicle switching.

The final `./bin/test` passed 46 Qt checks plus the Lua, Node, CLI, and standalone lifecycle suites. Visual evidence and viewport checks are in [the UI audit](ui-audit.md).
