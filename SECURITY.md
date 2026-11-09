# Security and privacy

ABM is a local convenience tool, not a sandbox against code running as your user.

- The game receiver binds IPv4 **loopback**, UDP port 44444; never forward it
  through your router or expose it to the LAN.
- Local command/activation sockets use user-access restrictions. Requests are
  bounded and tied to the current game session. The bridge accepts an explicit
  action allowlist, not arbitrary Lua source.
- Numeric tuning values are finite/range checked; the bridge checks ownership
  for vehicle actions and stopping requirements for repair/garage operations.
- Plain text rendering prevents markup-shaped server/player names from becoming
  rich text or remote image requests. Dynamic values are not shell-interpolated.
- Datagram size (32 KiB), player/active-vehicle lists (128), catalog/config lists
  (2048), strings, requests, clients and output queues are bounded.
- There are no cloud accounts, analytics, chat logging, packet interception or
  automatic updates. The sole directory lookup fetches BeamMP's public HTTPS
  server roster without uploading telemetry, player lists or current-server
  identity. The directory host sees the requesting IP; no directory is cached
  on disk. Other state stays in runtime storage, memory and user preferences.

The [official-server guard](docs/server-policy.md) is cooperative, not tamper-proof.
Its short leases and frozen roster reduce accidental use on official events;
they cannot authenticate an unmodified client or detect an approved raw server
name being reused for an event or impersonated at another address. Approval is
name-only, not address-bound. Do not treat it as a staff/competition guarantee.

Other local processes may spoof UDP telemetry. User-only sockets and request
validation do not provide cryptographic authentication of game data. Omarchy
plugins run in the desktop shell process, which is not a security sandbox.

## Reporting

While this repository is private, report vulnerabilities to the owner through
an existing private GitHub channel. Do not publish exploit details, secrets or
personal logs. If no private contact channel is available, open a minimal issue
requesting one without including sensitive details.

Supported security fixes target the latest preview version; no long-term support
or audit certification is promised. ABM is not an anti-cheat bypass and does not
make multiplayer tuning permissible where server rules prohibit it.
