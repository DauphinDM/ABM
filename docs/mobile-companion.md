# Mobile companions (development)

The separate public [ABM Mobile repository](https://github.com/DauphinDM/ABM-Mobile) adapts PC ABM's Qt Quick dashboard for Android and adds an independent native SwiftUI app for iOS 26+. The iOS app uses system Liquid Glass navigation and controls. Both use the same **Connect** protocol. **You need PC ABM 0.13.0 or newer.** The older 0.12.0 release doesn't have the host. This change doesn't replace any existing release assets.

The same PC listener also serves a browser control page for older devices that can't install the mobile apps. It uses the same pairing secret, explicit PC approval, command allowlist, and game checks. See [browser control](#browser-control-for-older-devices).

Android development APKs build in CI. iOS provides simulator artifacts and an unsigned arm64 device IPA that needs your own local signing and provisioning. A simulator app can't be installed on an iPhone. See [native iOS notes](https://github.com/DauphinDM/ABM-Mobile/blob/main/docs/ios.md). An earlier locally re-signed iPhone build was installed over USB and paired over Wi-Fi. The new USB transport and remembered-device flow still need physical-device testing. This isn't general signing-tool or live-game certification.

## Pairing

1. Put your phone and PC on the same trusted private Wi-Fi network. Guest-network isolation can block communication.
2. On the PC, open **Connect**, select the private IPv4 interface, and enable phone access. It's off every time the app starts and never enabled in demo mode.
3. Copy the two-minute pairing link and transfer it to your phone privately. Paste it into mobile **Connect**, enter a device name, and request approval.
4. Check that you started the pairing, then approve it on the PC. Leave **Remember this device** selected to save revocable access. The name is self-reported, not an authenticated identity.
5. Later, enable the PC listener and tap **Reconnect** on the phone. A remembered device doesn't need a new pairing link. There is no automatic reconnect. **Stop** disconnects but keeps saved devices. **Forget all devices** on the PC revokes them and stops access. QR scanning and discovery are not included.

The PC needs Qt 6.8+ with a working TLS backend and the `openssl` executable on PATH to create the local certificate the first time. On Windows, that may mean a separate OpenSSL installation. The current Windows ZIP doesn't supply its CLI. Failures show up in Connect instead of falling back to plaintext. On Linux, private identity files are stored owner-only under the app config directory's `mobile/` subdirectory. On Windows, normal account configuration-directory ACLs apply. ABM doesn't automate ACL hardening.

## Browser control for older devices

1. Enable remote access in PC **Connect**. The existing HTTPS listener on port 44445 serves the browser page as well as the mobile apps.
2. Open the displayed **browser address** on the older phone, tablet or computer. It must be on the same trusted private network. Accept the self-signed certificate warning only for this PC and this network; a certificate warning over the internet is never acceptable.
3. Paste the same two-minute pairing link (or its 64-character secret) and enter a browser name. Request approval, then approve that browser on the PC. The browser and native phone cannot control ABM at the same time.
4. The page provides Drive telemetry, Workshop controls, Garage model/configuration selection with spawn/replace, Session roster/owned-vehicle switching and disconnect controls. It does not load Garage thumbnails or update checks. A dispatch receipt is not proof that the game accepted the request.

The browser session uses an HttpOnly, Secure, SameSite=Strict cookie and expires after 12 hours. The pairing secret is not stored in the page. If **Remember this device** was selected during PC approval, the browser may store a revocable token in local storage for a **Resume saved browser** action; use **Forget saved browser** to remove it from that browser and **Forget all devices** on the PC to revoke it everywhere. Stopping access closes the listener immediately. All actions still require a live game, the exact frozen target, an owner-permitted stopped vehicle where applicable, and server policy permission.

## Firewall setup and a stuck secure connection

If the phone stays on **Connecting securely to PC**, first check that PC Connect says it's listening, that the link is fresh, that iPhone Local Network permission is allowed, and that both devices are on the same trusted LAN. A listening socket doesn't prove that a firewall lets the phone reach it. Guest Wi-Fi isolation can also block traffic.

PC Connect includes **Firewall & connection help → Allow LAN pairing through firewall…**. Nothing runs automatically. Review the displayed subnet, interface, PC address, and port. Confirm, then approve the OS administrator prompt.

- **Linux with UFW and pkexec:** a fixed-argument UFW rule allows inbound TCP from the selected interface's actual subnet to its bound PC address and port, on that interface only. ABM doesn't enable or disable UFW, reset its rules, or change other firewall managers. Other Linux firewalls need manual setup. Missing tools and unusual network masks are reported instead of guessing a scope.
- **Windows:** the UAC-approved Windows Firewall rule is limited to the ABM executable, the bound IPv4 address and port, the selected local subnet, and **Private** network profiles. ABM doesn't mark a Public network as Private for you. Only choose Private in Windows Settings for a network you actually trust. Moving the portable app or changing LAN addresses may need a new rule.
- Automatic subnet setup accepts real private IPv4 masks from /16 to /30. Public or loopback binds, overly broad masks, and unknown interfaces never produce an automatic rule. A subnet rule also allows other devices on that subnet. TLS, a fresh secret, and PC approval are still required. Administrators can add a narrower rule for one phone's current IP instead.
- The rule **stays** when phone access stops or ABM closes. Stopping closes the listener but keeps remembered devices and administrator-managed firewall rules. Remove the ABM rule in your firewall manager when you no longer need it. For UFW, review `sudo ufw status numbered` and delete only the matching rule. Finishing setup is not proof of end-to-end connectivity.

After firewall setup, stop and re-enable access for a **new two-minute link**, pair on the phone, and approve it on the PC. A canceled or failed administrator prompt leaves no confirmed setup. The app reports the failure instead of claiming the phone is connected.

**Never forward port 44445 to the internet**, disable the firewall to make pairing work, or expose ABM's game UDP or local control socket. Only private IPv4 is supported. IPv6 and cross-network relays are out of scope.

## Controls and restrictions

The phone offers telemetry, selected power/RPM/clutch/gear controls, recovery and reset actions, photo-backed model and configuration browsing, spawn and replace, session copies, and owned-vehicle switching. Tapping Apply sends directly. The redundant "Send to PC" dialog is gone, but that's not PC security approval. Changes need a known stopped, owned car and a permitted server. **Spawn into a loaded empty map** doesn't need an already-selected car. It still needs a valid current configuration and allowed policy. Backend, receiver, and game checks remain. Both screens share pending-action state, so simultaneous edits can show as busy. Nothing is queued offline.

Keyboard dismissal uses **Done** (native iOS keyboard toolbar or Android header), iOS swipe-to-dismiss, and dismissal when you change tabs or apply. Small current-library JPEG thumbnails are transferred on demand. Missing or invalid images fall back honestly. Telemetry runs at up to 10 Hz without resending the library or queuing stale updates. **Phone ↔ PC** measures transport round-trip time, not BeamMP game ping, and it isn't a latency guarantee.

This is not a steering wheel or throttle controller. Remaining gaps include full preset-file management, forum lookups, dimensions, full handling and engine editors, and desktop setup. A dispatch receipt is not proof that the game accepted the action. Readback and game results are still the source of truth. Physical lifecycle and live BeamNG/BeamMP testing are still required.

## USB mode (development)

Select **iPhone USB** or **Android USB** in PC Connect before enabling access. USB binds PC TLS only to `127.0.0.1:44445`, so no LAN firewall opening is needed. TLS pinning and initial approval stay the same. Keep one intended phone attached and the mobile app in the foreground. ABM doesn't install drivers or enable debugging automatically.

- **iPhone:** install the optional `iproxy`/libusbmuxd helper, unlock the phone, trust the PC, and enable USB on the phone. PC iproxy connects local port 44448 to the phone's loopback relay on 44446. Its local TLS client uses 44447. Free those fixed ports first. The PC and phone byte relays don't decrypt TLS. Windows needs compatible helpers and drivers. Physical support isn't certified.
- **Android:** install optional adb, enable USB debugging yourself, and authorize exactly one device. ABM resolves its serial, refuses an existing reverse mapping on 44445, and uses `--no-rebind`. On stop, it removes only its matching mapping on that same serial. It never removes all mappings or another device's rules.
- First-time USB pairing uses the USB-specific link. A PC remembered over Wi-Fi can be resumed over USB with the same pin and token. A USB-first saved endpoint is loopback. Pair once over Wi-Fi to learn that PC's LAN endpoint.

Automated tests exercise TLS through the PC byte bridge, the phone loopback relay, and a non-destructive mocked adb lifecycle. They are not physical cable, driver, or performance tests. USB may reduce Wi-Fi variability. It doesn't remove game processing time or imply continuous steering support.

## Security boundary

- TLS 1.2 or newer, with the PC certificate fingerprint in the invitation. The mobile client checks the pin before sending the secret. Self-signed certificates without the matching pin are refused.
- First pairing uses a fresh 32-byte secret, a two-minute invitation, and explicit approval. Optional remembered tokens allow explicit resume while PC access is enabled. The PC stores only hashes, at most 16. iOS keeps tokens in the device-only Keychain. Android keeps them in private, non-backed-up storage. Forget on the PC revokes them. There is no auto-reconnect. Links and tokens are capabilities. Keep them private.
- This is a separate protocol, not remote access to the local receiver. Only fixed actions are available. There is no arbitrary Lua, shell, file access, launcher, or setup endpoint.
- Each command carries the bridge session, vehicle, workshop generation, map, and server identity. Target changes reject stale requests. Sequences are monotonic. The limits are ten game commands per second and 64 total authenticated messages per second.
- Snapshots whitelist game state and strip image paths. Desktop installation paths, local notes, and preferences are excluded. Gameplay, player, and server information is still personal data. Only pair a device you trust.
- TLS handshakes, unauthenticated connections, input, and output are capped. Private-address filtering is not a substitute for authentication, and it doesn't prevent every denial of service on a hostile LAN.
- The phone disables controls and clears state if snapshots stop for more than two seconds. Stopping or disconnecting cancels queued live-power edits. Effects the game already accepted are not rolled back automatically.

See the mobile repository's [wire-protocol notes](https://github.com/DauphinDM/ABM-Mobile/blob/main/docs/protocol.md). This is an experimental boundary, not a security audit or a claim of internet-service hardening.
