# Security and privacy

## Development: mobile LAN access

The new optional Connect host is off by default and separate from the loopback game receiver. It binds one selected private IPv4 address and requires TLS certificate pinning, a fresh short-lived pairing secret and explicit PC approval. Only fixed validated actions and whitelisted game state are exposed; existing policy/ownership checks remain. See [mobile security boundaries and known limits](docs/mobile-companion.md). Never forward port 44445 or publish pairing links/private keys. Approval lasts one connection; stop access to revoke. This is experimental and has not been independently audited.

Connect's optional firewall setup runs only after an explicit scope confirmation and OS administrator approval. Linux UFW rules are bound to the selected interface, actual private subnet and listener address/TCP port; Windows rules additionally restrict the executable and Private network profile. It never disables a firewall, creates router forwards, broadens to all networks or silently changes unrelated rules. These permissions persist until removed in the OS firewall manager; stopping Connect closes the listener and revokes phone approval, not the firewall rule. Unsupported tools/masks require manual review. A firewall setup result is not a verified connection.

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
- Previews read only approved local game/user roots, without extracting archives
  to disk. Candidate count, encoded bytes, image dimensions, archive scans and
  the in-memory thumbnail cache are bounded; see [architecture](docs/architecture.md).
- There are no ABM cloud accounts, analytics, chat logging, packet interception or
  automatic updates. The directory lookup fetches BeamMP's public HTTPS
  server roster without uploading telemetry, player lists or current-server
  identity. The directory host sees the requesting IP; no directory is cached
  on disk.
- Explicit account-ID copying can request a public username profile from
  `https://forum.beammp.com`; opening a profile uses the default browser. These
  actions disclose the requested username and requesting IP to the forum, not
  the complete player list or game telemetry. Lookup requests have a ten-second
  timeout and 256 KiB response limit and do not follow redirects. A forum name
  mapping does not prove ownership of the connected player's account.
- Other state stays in runtime storage, memory, user preferences and ABM-owned
  preset files. Presets contain validated vehicle settings, not saved permission
  grants or executable code; they are not automatically applied on startup.

The [official-server guard](docs/server-policy.md) is cooperative, not tamper-proof.
Its short leases and frozen roster reduce accidental use on official events;
they cannot authenticate an unmodified client or detect an approved raw server
name being reused for an event or impersonated at another address. Approval is
name-only, not address-bound. Do not treat it as a staff/competition guarantee.

Other local processes may spoof UDP telemetry. User-only sockets and request
validation do not provide cryptographic authentication of game data. Omarchy
plugins run in the desktop shell process, which is not a security sandbox.

## Reporting

Report vulnerabilities through GitHub's private vulnerability reporting on the
repository's **Security** tab, or an existing private channel with the owner.
Do not publish exploit details, secrets or personal logs. If no private contact
channel is available, open a minimal issue requesting one without sensitive details.

Supported security fixes target the latest preview version; no long-term support
or audit certification is promised. ABM is not an anti-cheat bypass and does not
make multiplayer tuning permissible where server rules prohibit it.
