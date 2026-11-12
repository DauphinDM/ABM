# Session action review

## API contracts consulted

- [BeamMP client-side reference](https://docs.beammp.com/API%20documentation/Client-Side/):
  `extensions.MPCoreNetwork.getCurrentServer()` returns the current endpoint and
  name; `extensions.MPVehicleGE.getPlayers()` is keyed by session player ID;
  `isOwn()` takes a **game vehicle ID**, not a server vehicle string. The bridge
  now uses the same module resolution for snapshots and authorization, preferring
  documented extension modules and retaining old global aliases as a fallback.
- [BeamMP server reference](https://docs.beammp.com/scripting/server/latest-server-reference/):
  player identifiers are a server-side API. Session player IDs are not permanent
  account IDs. The public forum username JSON lookup is separate and does not
  verify that the person in a game owns that forum account. No client account-ID
  or authentication API is invented here; missing IDs do not imply guest status.
- [BeamNG virtual machines](https://documentation.beamng.com/modding/programming/virtualmachines/):
  GE and vehicle Lua state are separate. Roster/selection stay in GE; resetting
  ABM trip statistics stays entirely in the local receiver, not the vehicle VM.

## Defects addressed

Some ownership and command paths used only legacy BeamMP globals while snapshots
already supported extension modules. With only the documented namespace, owned
vehicles could disappear from the selectable list. These paths now share module
resolution and still fail closed when multiplayer ownership is unavailable.
Roster conversion also ignored the documented player-map key and inferred guest
status from missing embedded IDs; the two concepts are now kept separate.

Profile state could outlive invalid telemetry snapshots, and pending lookups were
not immediately canceled when the server or a same-ID player's name changed.
The backend now updates the profile roster before notifying QML, includes the
server identity in lookup context, and clears it on disconnect/invalid snapshots.
Forum lookup results are bounded, exact-name checked, explicit-intent only and
never followed across redirects. Player action feedback is also shown on-card.

Sample players no longer open unrelated real forum pages. The owned-vehicle
dialog explains disabled selection; native selection API errors are reported,
and successful dispatch is described as a request, not verified game acceptance.

## Verification boundaries

The initial isolated test did **not** reproduce universal click failure: basic
copy and dialog buttons worked. It did expose an unrelated missing-engine-array
QML binding error, now guarded. Regression coverage exercises real QML presses
held across changing receiver telemetry, actual clipboard contents, browser URL
dispatch intercepted locally, local trip reset and confirmed selection dispatch.
Forum success, malformed/oversized/mismatched/redirect responses and late-copy
cancellation use a fake network manager, never real usernames or forum traffic.
Lua mocks cover extension-only modules, player-map keys including ID zero,
ownership refusal, unverified-policy refusal and native selection exceptions.

No live game session, real browser launch, package installation or user game
files were changed. Live BeamNG/BeamMP behavior remains to be checked after
explicitly updating/loading the new bridge; passing isolated tests is not a
claim that every server or mod accepts vehicle switching.

Final `./bin/test` passed 46 Qt checks and the Lua, Node, CLI and standalone
lifecycle suites. Visual evidence and viewport checks are recorded in
[the UI audit](ui-audit.md).
