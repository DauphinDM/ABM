# Android companion — development

The separate public [ABM Mobile repository](https://github.com/DauphinDM/ABM-Mobile) adapts PC ABM's Qt Quick dashboard for Android. Both apps now have a **Connect** screen in source. **Released PC 0.12.0 does not contain this host.** Build the updated PC source before trying the mobile development APK. No existing release assets are replaced by this change.

## Pairing

1. Put phone and PC on the same trusted private Wi-Fi network. Guest-network isolation may prevent communication.
2. On PC, open **Connect**, select the private IPv4 interface and enable phone access. It is off on every app start and never enabled in demo mode.
3. Copy the two-minute pairing link and transfer it privately to your phone. Paste it in mobile **Connect**, enter a device name and request approval.
4. Check that you just initiated pairing, then approve on PC. The name is self-reported, not an authenticated identity.
5. Stop access on PC to revoke immediately. After disconnection, stop/enable again and pair anew. Approval is not saved. QR scanning and automatic discovery are not included yet.

The PC needs Qt 6.8+ with a working TLS backend and the `openssl` executable on PATH for first-time local certificate creation. On Windows this may require a separate OpenSSL installation; the current Windows ZIP does not supply its CLI. Failures appear in Connect instead of falling back to plaintext. Linux private identity files are stored owner-only under the app configuration directory's `mobile/` subdirectory. On Windows the normal account configuration-directory ACLs apply; ACL hardening is not automated.

If necessary, allow only private-network TCP access to the chosen PC interface on port **44445**. No firewall settings are changed for you. **Never forward this port to the Internet**, and never expose ABM's game UDP or local control socket. Only private IPv4 is supported in this first version; IPv6 and cross-network relays are out of scope.

## Controls and restrictions

The phone currently offers telemetry, selected power/RPM/clutch/gear controls, recovery/reset actions, model/configuration browsing, confirmed spawn/replace, session copies and owned-vehicle switching. Mobile changes require a known stopped, owned car and permitted server, even for some actions less restricted on desktop. Existing backend, receiver and game checks remain in place. PC and phone share the same current vehicle and pending-action state; simultaneous edits can be rejected as busy. No command is queued offline.

This is not a steering wheel or throttle controller. Initial mobile gaps include photos, full preset-file management, profile lookups, dimension experiments, full handling/engine editors and desktop setup. A dispatch receipt is not verified gameplay acceptance; readback/game result messages remain authoritative. Actual phone/Android lifecycle and live BeamNG/BeamMP validation are still required.

## Security boundary

- TLS 1.2 or newer, with the PC certificate fingerprint in the invitation. The mobile client checks the pin before transmitting the secret. Self-signed certificates without the matching pin are refused.
- A fresh 32-byte random secret, two-minute invitation, explicit PC approval, one approved connection, no saved credentials or auto-reconnect. The link is a capability and must be kept private; clipboard/transfer applications may retain it.
- A separate protocol, not remote access to the local receiver. Fixed actions only, no arbitrary Lua, shell, file access, launcher or setup endpoints.
- Each command carries bridge session, vehicle, workshop generation, map and server identity. Target changes reject stale requests. Monotonic sequences and a ten-command/second limit apply.
- Snapshots whitelist game state and strip image paths, excluding desktop installation paths, local notes and preferences. Gameplay/player/server information is still personal data; only pair a trusted device.
- TLS handshakes, unauthenticated connections, input and output are bounded. Private-address filtering is not a substitute for authentication and does not prevent all denial of service on a hostile LAN.
- The phone disables controls/clears state if snapshots stop for more than two seconds. Stopping/disconnecting cancels queued live-power edits; already accepted game effects are not rolled back automatically.

See the mobile repository's [wire-protocol notes](https://github.com/DauphinDM/ABM-Mobile/blob/main/docs/protocol.md). This is an experimental boundary, not a security audit or Internet-service hardening claim.
