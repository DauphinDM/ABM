# Official-server protection (development 0.7.1)

ABM freezes the **108 regular official server names observed on 9 November 2026**. The four official BeamMP × MOZA qualifying servers are excluded. The snapshot lives in `game/lua/ge/extensions/abm-official-servers.json`. ABM doesn't silently expand it when the directory changes.

Matching uses the **exact raw name only**, including case, spacing, and any formatting codes. IP addresses and ports in the snapshot are historical capture metadata, not allowlist criteria.

| Connected server | Gameplay changes |
| --- | --- |
| Official, exact approved raw name, at any IP or port | Allowed |
| Official event, new official, changed or unapproved name | Blocked |
| Verified non-official | No extra ABM official-server restriction |
| Joining, missing identity, missing or stale directory, failed HTTPS request | Locked until verified |

While connected to multiplayer, the receiver reads `https://backend.beammp.com/servers-info` over verified HTTPS at most once every 30 seconds. Results expire after 60 seconds. A failed request invalidates them immediately. ABM doesn't upload the connected server identity, telemetry, or player list. The directory host sees your normal requesting IP. There is no persistent directory cache.

Both the local command gateway and the game bridge enforce the policy. The bridge binds a short permission lease, at most three seconds, to the current session, exact endpoint, and raw name. It also checks its own bundled frozen **name** roster. Endpoint matching links the live directory record and lease to the connected server; it doesn't compare against historical addresses. After a host or port rotation, the same approved name can be classified fresh and allowed. Old leases can't authorize another connection.

Missing files or bridges older than 0.7.1 don't unlock multiplayer actions. There is no UI override and no automatic roster enrollment. Install **all four bridge files** and restart BeamNG when convenient. Reloading GE Lua alone can't replace an already loaded vehicle extension or make older changes safe.

## When permission changes

Read-only telemetry, catalog and configuration browsing, and **Stop bouncing** stay available. Gameplay-changing requests are blocked, including garage actions, repair, and the whole sandbox workshop. Periodic power and protection reapply stops. ABM cancels pending requests, restores captured gravity and ABM-owned engine, handling, and gearbox values, stops bouncing, and requests a physics reset for vehicles it modified that are still locally owned. Cleanup failures keep changes locked and are retried. You may need to reset or reload.

**Cleanup can reset your car even while you're moving.** That's a deliberate safety tradeoff, and it also applies to an interrupted directory connection. The callback confirms that restoration code ran and a reset was requested, not that the server certified the resulting physics. Previously spawned cars aren't deleted, and unrelated mod or configuration changes aren't undone. Restart and reset before relying on protection after upgrading from an older ABM bridge.

## Limits: not anti-cheat or authorization

- This is a cooperative client restriction, not tamper-proof anti-cheat. A modified app, forged local UDP traffic, or another tool can bypass it.
- Official classification depends on the live BeamMP directory. A newly official server that's incorrectly marked non-official there can't be reliably identified unless its name is already in the frozen roster. A former official address reused by a verified non-official server isn't restricted.
- Repurposing or impersonating an approved server **without changing its raw name**, even at another address, looks identical under this name-only policy. ABM can't detect that an approved regular name is now used for an event.
- Non-official does not mean tuning is allowed. Server rules still apply.
- Automated mocks cover policy decisions and cleanup. Live transition and reset behavior still need in-game verification. This is not a promise against moderation action, staff consequences, or competition disqualification.

Roster changes must be a reviewed source change, never a downloaded allowlist update. The published private 0.4 preview doesn't contain this guard.
