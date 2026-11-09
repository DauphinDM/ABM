# Official-server protection (development 0.7.1)

ABM freezes the **108 regular official server names observed on 9 November 2026**.
The four official BeamMP × MOZA qualifying servers are explicitly excluded.
The snapshot lives in `game/lua/ge/extensions/abm-official-servers.json`; it is
not silently expanded when the directory changes. Matching is **exact raw name
only**, including case, spacing and any formatting codes. IP addresses and ports
in the snapshot are historical capture metadata, not allowlist criteria.

| Connected server | Gameplay changes |
| --- | --- |
| Official, exact approved raw name, at any IP/port | Allowed |
| Official event, new official, changed/unapproved name | Blocked |
| Verified non-official | No additional ABM official-server restriction |
| Joining, missing identity, missing/stale directory, failed HTTPS request | Locked until verified |

The receiver reads `https://backend.beammp.com/servers-info` over verified HTTPS
at most once every 30 seconds while connected to multiplayer. Results expire
after 60 seconds; request failures invalidate them immediately. No connected
server identity, telemetry or player list is uploaded. The directory host sees
the normal requesting IP address. There is no persistent directory cache.

Both the local command gateway and the game bridge enforce the policy. The
bridge binds a short permission lease (at most three seconds) to the current
session, exact endpoint and raw name, and independently checks its bundled
frozen **name** roster. Endpoint matching associates the live directory record
and lease with the connected server; it does not compare against historical
addresses. After a host/port rotation, a fresh classification of the same
approved name is allowed. Old leases cannot authorize another connection.
Missing files or pre-0.7.1 bridges do not unlock multiplayer actions.
There is no UI override or automatic roster enrollment. Install **all four
bridge files** and restart BeamNG when convenient; reloading GE Lua alone cannot
replace an already loaded vehicle extension or make older changes safe.

## When permission changes

Read-only telemetry, catalog/config browsing and **Stop bouncing** remain
available. Gameplay-changing requests, including garage actions, repair and
the entire sandbox workshop, are blocked. Periodic power/protection reapply
stops. ABM cancels pending requests, restores captured gravity and ABM-owned
engine/handling/gearbox values, stops bouncing, and requests a physics reset
for vehicles it modified that are still locally owned. Cleanup failures keep
changes locked and are retried; a reset/reload may be necessary.

**Cleanup can reset your car even while moving.** This intentional safety
tradeoff also applies to an interrupted directory connection. The callback
confirms restoration code ran and a reset was requested, not that the server
has certified the resulting physics. Previously spawned cars are not deleted,
and unrelated mods/configuration changes are not undone. Restart and reset
before relying on protection after upgrading from an older ABM bridge.

## Limits — not anti-cheat or authorization

- This is a cooperative client restriction, not tamper-proof anti-cheat.
  A modified app, forged local UDP traffic or another tool can bypass it.
- Official classification depends on the live BeamMP directory. A newly
  official server incorrectly marked non-official there cannot be reliably
  identified unless its name is already known to the frozen roster. A former
  official address reused by a verified non-official server is not restricted.
- Repurposing or impersonating an approved server **without changing its raw
  name**, even at another address, is indistinguishable under this name-only
  policy. ABM cannot detect that an approved regular name is now used for an event.
- Non-official does not mean tuning is permitted. Server rules still apply.
- Automated mocks cover policy decisions and cleanup. Live transition/reset
  behavior still needs in-game verification. This is not a promise against
  moderation action, staff consequences, or competition disqualification.

Roster changes must be an explicit reviewed source change, never a downloaded
allowlist update. The published private 0.4 preview does not contain this guard.
