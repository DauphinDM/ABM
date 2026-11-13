# Security and privacy

## Development: app updates

Update checks are opt-in requests to fixed official GitHub endpoints. They are separate from remote game commands. PC and Android downloads need a size limit and SHA-256 check before atomic acceptance, with restricted redirects and a second hash check before the OS takes over. No downloaded code, privileged installer, or game shutdown runs automatically. Android gives its installer only a private, read-only APK URI. iOS only opens the official release page. A compromised repository is still a supply-chain risk. See [update boundaries](docs/updates.md).

## Development: mobile LAN access

The optional Connect host is off by default and separate from the loopback game receiver. It binds one selected private IPv4 address, or loopback for explicit USB mode. First pairing needs certificate-pinned TLS, a fresh short-lived secret, and PC approval. If you choose Remember, a revocable random token lets you reconnect later while the host is enabled. The PC stores only the token hash. iOS uses the device-only Keychain, and Android uses private, non-backed-up app storage. Stop disconnects but keeps remembered devices. **Forget all devices** revokes their tokens. Treat saved tokens like passwords. Existing policy and ownership checks still apply. Never forward port 44445 or publish links, tokens, or keys. This is experimental and hasn't been independently audited. See [mobile boundaries](docs/mobile-companion.md).

Connect's optional firewall setup runs only after you confirm the scope and approve the OS administrator prompt. Linux UFW rules are bound to the selected interface, actual private subnet, and listener address and TCP port. Windows rules also limit the executable and require Private network profiles. ABM never disables a firewall, creates router forwards, broadens a rule to all networks, or silently changes unrelated rules. These permissions stay until you remove them in the OS firewall manager. Stopping Connect closes the listener and ends the current connection, but keeps remembered tokens and the firewall rule. Unsupported tools or masks need manual review. A firewall setup result is not a verified connection.

ABM is a local convenience tool. It is not a sandbox against code already running as your user.

USB uses optional unprivileged iproxy or adb helpers and size-limited loopback byte relays. It keeps end-to-end TLS and PC approval. It doesn't install drivers, enable USB debugging, or open a LAN firewall rule. Android refuses existing reverse mappings and removes only its own matching mapping on the selected device. Stopping Connect closes the listener but does not revoke remembered-device tokens or remove administrator-managed firewall rules.

- The game receiver binds IPv4 **loopback**, UDP port 44444. Never forward it through your router or expose it to the LAN.
- Local command and activation sockets use user-access restrictions. Requests are size-limited and tied to the current game session. The bridge accepts an explicit action allowlist, not arbitrary Lua source.
- Numeric tuning values are checked for finite numbers and valid ranges. The bridge checks ownership for vehicle actions and stopping requirements for repair and garage operations.
- Plain-text rendering stops markup-shaped server or player names from becoming rich text or triggering remote image requests. Dynamic values are never shell-interpolated.
- Datagram size (32 KiB), player and active-vehicle lists (128), catalog and configuration lists (2048), strings, requests, clients, and output queues all have limits.
- Previews read only approved local game and user roots, without extracting archives to disk. Candidate count, encoded bytes, image dimensions, archive scans, and the in-memory thumbnail cache are limited. See [architecture](docs/architecture.md).
- There are no ABM cloud accounts, analytics, chat logging, packet interception, or automatic updates. The directory lookup fetches BeamMP's public HTTPS server roster without uploading telemetry, player lists, or current-server identity. The directory host sees the requesting IP. No directory is cached on disk.
- Explicit account-ID copying can request a public username profile from `https://forum.beammp.com`. Opening a profile uses your default browser. These actions disclose the requested username and your IP to the forum, not the complete player list or game telemetry. Lookup requests time out after ten seconds, cap responses at 256 KiB, and don't follow redirects. A forum name mapping doesn't prove that the connected player owns that account.
- Other state stays in runtime storage, memory, user preferences, and ABM-owned preset files. Presets contain validated vehicle settings, not saved permission grants or executable code. They are not applied automatically at startup.

The [official-server guard](docs/server-policy.md) is cooperative, not tamper-proof. Its short leases and frozen roster reduce accidental use on official events. They can't authenticate an unmodified client or detect an approved raw server name being reused for an event or impersonated at another address. Approval is name-only, not address-bound. Don't treat it as a staff or competition guarantee.

Other local processes can spoof UDP telemetry. User-only sockets and request validation don't cryptographically authenticate game data. Omarchy plugins run in the desktop shell process, which is not a security sandbox.

## Reporting

Report vulnerabilities through GitHub's private vulnerability reporting on the repository's **Security** tab, or through an existing private channel with the owner. Don't publish exploit details, secrets, or personal logs. If no private contact channel is available, open a minimal issue asking for one, without sensitive details.

Supported security fixes target the latest preview version. There is no long-term support or audit certification. ABM is not an anti-cheat bypass, and it doesn't make multiplayer tuning permissible where server rules prohibit it.
