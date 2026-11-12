# Security and privacy

## Development: app updates

Update checks are opt-in requests to fixed official GitHub endpoints, separate from remote game commands. PC/Android downloads require bounded size and SHA-256 before atomic acceptance, with restricted redirects and a second hash check before OS handoff. No downloaded code, privileged installer or game shutdown runs automatically. Android grants only a private, read-only APK URI to its installer; iOS only opens the official release page. Repository compromise remains a supply-chain risk. See [update boundaries](docs/updates.md).

## Development: mobile LAN access

The optional Connect host is off by default and separate from the loopback game receiver. It binds one selected private IPv4 address, or loopback for explicit USB mode. First pairing requires certificate-pinned TLS, a fresh short-lived secret and PC approval. If Remember is selected, a revocable random token permits later explicit reconnects while the host is enabled. PC stores only its hash; iOS uses device-only Keychain and Android private non-backed-up app storage. Stop disconnects but retains remembered devices; **Forget all devices** revokes their tokens. Treat saved tokens like passwords. Existing policy/ownership checks remain. Never forward port 44445 or publish links, tokens or keys. This is experimental and not independently audited; see [mobile boundaries](docs/mobile-companion.md).

Connect's optional firewall setup runs only after an explicit scope confirmation and OS administrator approval. Linux UFW rules are bound to the selected interface, actual private subnet and listener address/TCP port; Windows rules additionally restrict the executable and Private network profile. It never disables a firewall, creates router forwards, broadens to all networks or silently changes unrelated rules. These permissions persist until removed in the OS firewall manager; stopping Connect closes the listener and ends the current connection, but retains remembered tokens and the firewall rule. Unsupported tools/masks require manual review. A firewall setup result is not a verified connection.

ABM is a local convenience tool, not a sandbox against code running as your user.

USB uses optional unprivileged iproxy/adb helpers and bounded loopback byte relays, retaining end-to-end TLS and PC approval. It does not install drivers, enable USB debugging or open a LAN firewall rule. Android refuses existing reverse mappings and removes only its matching mapping on the same selected device. Stopping Connect closes the listener but does not revoke remembered-device tokens or remove administrator-managed firewall rules.

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
