# Mobile companions — development

The separate public [ABM Mobile repository](https://github.com/DauphinDM/ABM-Mobile) adapts PC ABM's Qt Quick dashboard for Android and adds an independent native SwiftUI implementation for iOS 26+, with system Liquid Glass navigation and controls. Both use the same **Connect** protocol. **Released PC 0.12.0 does not contain this host.** Build the updated PC source before trying the mobile development builds. No existing release assets are replaced by this change.

Android development APKs build in CI. iOS provides both simulator artifacts and an unsigned arm64 device IPA requiring your own local signing/provisioning; the simulator app is not installable on an iPhone. See [native iOS build notes](https://github.com/DauphinDM/ABM-Mobile/blob/main/docs/ios.md). One locally re-signed iPhone build has been installed successfully over USB; that is not certification of other signing tools, pairing or live-game operation.

## Pairing

1. Put phone and PC on the same trusted private Wi-Fi network. Guest-network isolation may prevent communication.
2. On PC, open **Connect**, select the private IPv4 interface and enable phone access. It is off on every app start and never enabled in demo mode.
3. Copy the two-minute pairing link and transfer it privately to your phone. Paste it in mobile **Connect**, enter a device name and request approval.
4. Check that you just initiated pairing, then approve on PC. The name is self-reported, not an authenticated identity.
5. Stop access on PC to revoke immediately. After disconnection, stop/enable again and pair anew. Approval is not saved. QR scanning and automatic discovery are not included yet.

The PC needs Qt 6.8+ with a working TLS backend and the `openssl` executable on PATH for first-time local certificate creation. On Windows this may require a separate OpenSSL installation; the current Windows ZIP does not supply its CLI. Failures appear in Connect instead of falling back to plaintext. Linux private identity files are stored owner-only under the app configuration directory's `mobile/` subdirectory. On Windows the normal account configuration-directory ACLs apply; ACL hardening is not automated.

## Firewall setup and a stuck secure connection

If the phone stays on **Connecting securely to PC**, first check that PC Connect says it is listening, that the link is fresh, that iPhone Local Network permission is allowed and that the devices are on the same trusted LAN. A listening socket does not prove that a firewall permits the phone to reach it. Guest Wi-Fi isolation can also block traffic.

PC Connect includes **Firewall & connection help → Allow LAN pairing through firewall…**. Nothing runs automatically: review the displayed subnet/interface/PC address/port, confirm, then approve the OS administrator prompt.

- **Linux with UFW + pkexec:** a fixed-argument UFW allow rule permits inbound TCP from the selected interface's actual subnet to its bound PC address/port, on that interface only. ABM does not enable/disable UFW, reset its rules, or change other firewall managers. Other Linux firewalls need manual setup; missing tools and unusual network masks are reported instead of guessing a scope.
- **Windows:** the UAC-approved Windows Firewall rule is restricted to the ABM executable, bound IPv4 address/port, the selected local subnet, and **Private** network profiles. ABM does not mark a Public network Private for you. Only select Private in Windows Settings for a network you actually trust. Relocating the portable app or changing LAN addresses may require a new rule.
- Automatic subnet setup accepts actual private IPv4 masks from /16 to /30. Public/loopback binds, overly broad masks and unknown interfaces never produce an automatic rule. A subnet rule allows other devices on that subnet too; TLS, a fresh secret and PC approval are still required. Administrators can instead add a narrower rule for one phone's current IP.
- The rule **persists** when phone access stops or ABM closes. Stopping access closes the listener and revokes approvals, but does not silently alter administrator-managed firewall rules. Remove the ABM rule in your firewall manager when no longer needed (for UFW, review `sudo ufw status numbered` and delete only the matching rule). A completed setup request is not proof of end-to-end connectivity.

After firewall setup, stop/re-enable access for a **new two-minute link**, pair on the phone, and approve it on PC. A canceled/failed administrator prompt leaves no confirmed setup; the app reports the failure instead of claiming the phone is connected.

**Never forward port 44445 to the Internet**, disable the firewall to make pairing work, or expose ABM's game UDP/local control socket. Only private IPv4 is supported; IPv6 and cross-network relays remain out of scope.

## Controls and restrictions

The phone currently offers telemetry, selected power/RPM/clutch/gear controls, recovery/reset actions, model/configuration browsing, confirmed spawn/replace, session copies and owned-vehicle switching. Mobile changes require a known stopped, owned car and permitted server, even for some actions less restricted on desktop. Existing backend, receiver and game checks remain in place. PC and phone share the same current vehicle and pending-action state; simultaneous edits can be rejected as busy. No command is queued offline.

This is not a steering wheel or throttle controller. Initial mobile gaps include photos, full preset-file management, profile lookups, dimension experiments, full handling/engine editors and desktop setup. A dispatch receipt is not verified gameplay acceptance; readback/game result messages remain authoritative. Actual Android/iPhone lifecycle and live BeamNG/BeamMP validation are still required.

## Security boundary

- TLS 1.2 or newer, with the PC certificate fingerprint in the invitation. The mobile client checks the pin before transmitting the secret. Self-signed certificates without the matching pin are refused.
- A fresh 32-byte random secret, two-minute invitation, explicit PC approval, one approved connection, no saved credentials or auto-reconnect. The link is a capability and must be kept private; clipboard/transfer applications may retain it.
- A separate protocol, not remote access to the local receiver. Fixed actions only, no arbitrary Lua, shell, file access, launcher or setup endpoints.
- Each command carries bridge session, vehicle, workshop generation, map and server identity. Target changes reject stale requests. Monotonic sequences and a ten-command/second limit apply.
- Snapshots whitelist game state and strip image paths, excluding desktop installation paths, local notes and preferences. Gameplay/player/server information is still personal data; only pair a trusted device.
- TLS handshakes, unauthenticated connections, input and output are bounded. Private-address filtering is not a substitute for authentication and does not prevent all denial of service on a hostile LAN.
- The phone disables controls/clears state if snapshots stop for more than two seconds. Stopping/disconnecting cancels queued live-power edits; already accepted game effects are not rolled back automatically.

See the mobile repository's [wire-protocol notes](https://github.com/DauphinDM/ABM-Mobile/blob/main/docs/protocol.md). This is an experimental boundary, not a security audit or Internet-service hardening claim.
