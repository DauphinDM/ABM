# Mobile companions — development

The separate public [ABM Mobile repository](https://github.com/DauphinDM/ABM-Mobile) adapts PC ABM's Qt Quick dashboard for Android and adds an independent native SwiftUI implementation for iOS 26+, with system Liquid Glass navigation and controls. Both use the same **Connect** protocol. **PC ABM 0.13.0 or newer is required.** The older 0.12.0 release does not contain this host. No existing release assets are replaced by this change.

Android development APKs build in CI. iOS provides simulator artifacts and an unsigned arm64 device IPA requiring your own local signing/provisioning; a simulator app is not installable on an iPhone. See [native iOS notes](https://github.com/DauphinDM/ABM-Mobile/blob/main/docs/ios.md). An earlier locally re-signed iPhone build was installed over USB and paired over Wi-Fi. New USB transport and remembered-device operation still need physical-device testing; this is not general signing-tool or live-game certification.

## Pairing

1. Put phone and PC on the same trusted private Wi-Fi network. Guest-network isolation may prevent communication.
2. On PC, open **Connect**, select the private IPv4 interface and enable phone access. It is off on every app start and never enabled in demo mode.
3. Copy the two-minute pairing link and transfer it privately to your phone. Paste it in mobile **Connect**, enter a device name and request approval.
4. Check that you initiated pairing, then approve on PC. Leave **Remember this device** selected to save revocable access. The self-reported name is not an authenticated identity.
5. Later, enable the PC listener and tap **Reconnect** on the phone. No new pairing link is needed for a remembered device. There is no automatic reconnect. **Stop** disconnects but retains saved devices; **Forget all devices** on PC revokes them and stops access. QR scanning and discovery are not included.

The PC needs Qt 6.8+ with a working TLS backend and the `openssl` executable on PATH for first-time local certificate creation. On Windows this may require a separate OpenSSL installation; the current Windows ZIP does not supply its CLI. Failures appear in Connect instead of falling back to plaintext. Linux private identity files are stored owner-only under the app configuration directory's `mobile/` subdirectory. On Windows the normal account configuration-directory ACLs apply; ACL hardening is not automated.

## Firewall setup and a stuck secure connection

If the phone stays on **Connecting securely to PC**, first check that PC Connect says it is listening, that the link is fresh, that iPhone Local Network permission is allowed and that the devices are on the same trusted LAN. A listening socket does not prove that a firewall permits the phone to reach it. Guest Wi-Fi isolation can also block traffic.

PC Connect includes **Firewall & connection help → Allow LAN pairing through firewall…**. Nothing runs automatically: review the displayed subnet/interface/PC address/port, confirm, then approve the OS administrator prompt.

- **Linux with UFW + pkexec:** a fixed-argument UFW allow rule permits inbound TCP from the selected interface's actual subnet to its bound PC address/port, on that interface only. ABM does not enable/disable UFW, reset its rules, or change other firewall managers. Other Linux firewalls need manual setup; missing tools and unusual network masks are reported instead of guessing a scope.
- **Windows:** the UAC-approved Windows Firewall rule is restricted to the ABM executable, bound IPv4 address/port, the selected local subnet, and **Private** network profiles. ABM does not mark a Public network Private for you. Only select Private in Windows Settings for a network you actually trust. Relocating the portable app or changing LAN addresses may require a new rule.
- Automatic subnet setup accepts actual private IPv4 masks from /16 to /30. Public/loopback binds, overly broad masks and unknown interfaces never produce an automatic rule. A subnet rule allows other devices on that subnet too; TLS, a fresh secret and PC approval are still required. Administrators can instead add a narrower rule for one phone's current IP.
- The rule **persists** when phone access stops or ABM closes. Stopping closes the listener but retains remembered devices and administrator-managed firewall rules. Remove the ABM rule in your firewall manager when no longer needed (for UFW, review `sudo ufw status numbered` and delete only the matching rule). Setup completion is not proof of end-to-end connectivity.

After firewall setup, stop/re-enable access for a **new two-minute link**, pair on the phone, and approve it on PC. A canceled/failed administrator prompt leaves no confirmed setup; the app reports the failure instead of claiming the phone is connected.

**Never forward port 44445 to the Internet**, disable the firewall to make pairing work, or expose ABM's game UDP/local control socket. Only private IPv4 is supported; IPv6 and cross-network relays remain out of scope.

## Controls and restrictions

The phone offers telemetry, selected power/RPM/clutch/gear controls, recovery/reset actions, photo-backed model/configuration browsing, spawn/replace, session copies and owned-vehicle switching. Tapping Apply sends directly: redundant Send to PC dialogs are removed, not PC security approval. Changes require a known stopped, owned car and permitted server. **Spawn into a loaded empty map** does not require an already-selected car; it still needs a valid current configuration and allowed policy. Backend, receiver and game checks remain. Both screens share pending-action state, so simultaneous edits can be busy. No command is queued offline.

Keyboard dismissal uses **Done** (native iOS keyboard toolbar or Android header), iOS swipe-to-dismiss, and dismissal when changing tabs or applying. Small current-library JPEG thumbnails are transferred on demand; missing/invalid images fall back honestly. Telemetry runs at up to 10 Hz without repeatedly sending the library or queuing stale updates. **Phone ↔ PC** measures transport round-trip time, not BeamMP game ping or a latency guarantee.

This is not a steering wheel or throttle controller. Remaining gaps include full preset-file management, forum lookups, dimensions, full handling/engine editors and desktop setup. A dispatch receipt is not verified gameplay acceptance; readback/game results remain authoritative. Physical lifecycle and live BeamNG/BeamMP validation are still required.

## USB mode — development

Select **iPhone USB** or **Android USB** in PC Connect before enabling access. USB binds PC TLS only to `127.0.0.1:44445`; no LAN firewall opening is needed. TLS pinning and initial approval are unchanged. Keep one intended phone attached and the mobile app foregrounded. ABM does not install drivers or enable debugging automatically.

- **iPhone:** install the optional `iproxy`/libusbmuxd helper, unlock and trust the PC, and enable USB on the phone. PC iproxy connects local port 44448 to the phone's loopback relay on 44446; its local TLS client uses 44447. Free those fixed ports first. PC and phone byte relays do not decrypt TLS. Windows needs compatible helper/drivers; physical support is not certified.
- **Android:** install optional adb, enable USB debugging yourself and authorize exactly one device. ABM resolves its serial, refuses an existing reverse mapping on 44445 and uses `--no-rebind`. On stop it removes only its matching mapping on that same serial, never all mappings or another device's rules.
- First-time USB pairing uses the USB-specific link. A PC remembered over Wi-Fi can be resumed over USB with the same pin/token. A USB-first saved endpoint is loopback; pair once over Wi-Fi to learn that PC's LAN endpoint.

Automated tests exercise TLS through the PC byte bridge, phone loopback relay and non-destructive mocked adb lifecycle. They are not physical cable, driver or performance tests. USB may reduce Wi-Fi variability; it does not eliminate game processing or imply continuous steering support.

## Security boundary

- TLS 1.2 or newer, with the PC certificate fingerprint in the invitation. The mobile client checks the pin before transmitting the secret. Self-signed certificates without the matching pin are refused.
- First pairing uses a fresh 32-byte secret, two-minute invitation and explicit approval. Optional remembered tokens permit explicit resume while PC access is enabled; PC stores only hashes (at most 16), iOS device-only Keychain and Android private non-backed-up storage hold tokens. Forget on PC revokes them. No auto-reconnect. Links/tokens are capabilities; keep them private.
- A separate protocol, not remote access to the local receiver. Fixed actions only, no arbitrary Lua, shell, file access, launcher or setup endpoints.
- Each command carries bridge session, vehicle, workshop generation, map and server identity. Target changes reject stale requests. Monotonic sequences, ten game commands/second and 64 total authenticated messages/second apply.
- Snapshots whitelist game state and strip image paths, excluding desktop installation paths, local notes and preferences. Gameplay/player/server information is still personal data; only pair a trusted device.
- TLS handshakes, unauthenticated connections, input and output are bounded. Private-address filtering is not a substitute for authentication and does not prevent all denial of service on a hostile LAN.
- The phone disables controls/clears state if snapshots stop for more than two seconds. Stopping/disconnecting cancels queued live-power edits; already accepted game effects are not rolled back automatically.

See the mobile repository's [wire-protocol notes](https://github.com/DauphinDM/ABM-Mobile/blob/main/docs/protocol.md). This is an experimental boundary, not a security audit or Internet-service hardening claim.
