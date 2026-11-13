# ABM for Windows: standalone preview

This is a native Qt Quick application, not an Omarchy plugin and not a web app. The Windows build path and CI workflow are included, but Windows runtime support needs testing on Windows before it can be called stable. The Linux development machine can't certify Windows, Steam, or BeamMP behavior.

## Using a build

1. Extract the whole `ABM-<version>-Windows-x64.zip` release to a folder. Keep its DLLs, platform plugins, and QML files beside `ABM.exe`.
2. Run `ABM.exe`. You don't need Omarchy, Hyprland, Python, or systemd.
3. In **Settings → Paths & bridge setup**, confirm the BeamNG installation folder, its active **user folder** (the one containing `beamng.log`), and `BeamMP-Launcher.exe`. ABM finds defaults where it can. If you moved or have an old user folder, select it yourself.
4. Choose **Save & install bridge** once. Existing ABM-owned bridge files are backed up when updated. Unrelated files are not replaced.
5. When the game is closed, **Start BeamNG** starts it through Steam, and the bridge loads automatically. If the game was already open during setup, restart it yourself when convenient. ABM never restarts it.
6. While the game is running, the button becomes **Start BeamMP**. ABM enables the installed BeamMP game mod through a connected, updated bridge when needed, then starts the launcher with **`--no-launch`** from the launcher's own folder. Install BeamMP separately first. ABM doesn't download a launcher or mod.

The app starts its own localhost telemetry receiver when no other ABM receiver owns it. Closing the standalone window also closes the receiver it started. The game is never closed. If another service already provides the receiver, the app reuses it.

Windows uses per-user local named pipes for commands and IPv4 loopback UDP port 44444 for game telemetry. Desktop-only operation needs no inbound internet or LAN access. Don't open or forward this port on your router. Settings use Qt's per-user application config location with the `abm` application ID. The legacy game extension ID stays `beamdeck`. See the architecture guide for protocol compatibility details.

## Development: pairing a phone

ABM 0.13.0 adds **Connect**. The older 0.12.0 release doesn't include it. Creating the TLS identity the first time needs the `openssl` executable on PATH. The ZIP doesn't supply its CLI. Keep your phone and PC on a trusted LAN, enable access, transfer the fresh private link, and approve the phone on the PC.

If the phone stays on "Connecting securely," open **Firewall & connection help** on PC Connect. **Allow LAN pairing through firewall…** shows the actual subnet, bound address, and TCP port before asking for UAC approval. The rule is limited to this executable, that address and port, your local subnet, and **Private** network profiles. It never changes a Public network to Private or disables Windows Firewall. The rule stays until you remove it in Windows Firewall. Stopping Connect closes the listener and revokes phone approval. Canceling UAC is fine. Other security products or guest Wi-Fi isolation can still block access.

Use a new two-minute pairing link after setup. Never forward the companion port or the game receiver through your router. See [mobile setup](mobile-companion.md) for scope, limits, and removal guidance.

## Building

Use a Windows x64 MSVC developer shell with Qt 6.8.3 or newer (Desktop MSVC 2022 x64), Python 3 for CLI tests, and vcpkg. Install the archive reader:

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
& "$env:VCPKG_ROOT/vcpkg.exe" install libarchive:x64-windows
./bin/build-windows.ps1 -Test
```

`qmake`, `nmake`, and `windeployqt` must be on PATH. The script builds the app, runs offscreen Qt tests, deploys the Qt and archive DLLs into `dist/ABM`, and tests the packaged CLI and standalone receiver lifecycle with isolated settings. The `.github/workflows/windows.yml` workflow uploads the ZIP and SHA256 manifest as an artifact. It doesn't publish a release or need a signing key.

The portable folder isn't code-signed yet, so Windows SmartScreen may warn you. Don't disable Windows security globally. Dependency notices and Qt license texts are bundled. Signing and real-game testing on a clean Windows desktop are still outstanding. Never include proprietary vehicle assets in the ZIP. Previews are read from your own game installation.

Text-edit context menus support right-click and Shift+F10 on the bundled Qt 6.8 runtime. They don't need the newer Qt 6.9 text-edit menu APIs.

## Known boundaries

- Native Windows process detection and Steam lookup are implemented but need real-machine testing, including non-default Steam libraries.
- A launcher starting successfully is not a successful BeamMP server connection.
- A spawn acknowledgement means the request was submitted, not that the game accepted it.
- Custom configurations without pictures stay selectable with a clearly labeled model-preview fallback or "No preview available."
- Theme and language support fall back without Omarchy. There is no Windows bar plugin, background Windows service, or automatic updater in this preview.
