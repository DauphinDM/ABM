# ABM for Windows — standalone preview

This is a native Qt Quick application, not an Omarchy plugin and not a web app.
The Windows build path and CI workflow are included; Windows runtime support
must be verified on Windows before calling it a stable release. The Linux
development machine cannot certify Windows/Steam/BeamMP behavior.

## Using a build

1. Extract the entire `ABM-Windows-x64` artifact to a folder. Keep its DLLs,
   platform plugins and QML files beside `ABM.exe`.
2. Run `ABM.exe`. Omarchy, Hyprland, Python and systemd are not needed.
3. In **Settings → Paths & bridge setup**, confirm the BeamNG installation
   folder, its active **user folder** (the one containing `beamng.log`), and
   `BeamMP-Launcher.exe`. Defaults are discovered where possible; moved or old
   user folders must be selected explicitly.
4. Choose **Save & install bridge** once. Existing ABM-owned bridge files
   are backed up when updated; unrelated files are not replaced.
5. When the game is closed, **Start BeamNG** starts it through Steam. The bridge
   loads automatically. If it was already open during setup, restart the game
   yourself when convenient; the app never restarts it.
6. While the game is running, the button becomes **Start BeamMP**. It enables the
   installed BeamMP game mod through a connected, updated bridge when needed,
   then starts the launcher with **`--no-launch`** from the launcher's own folder.
   Install BeamMP separately first. The app does not download a launcher or mod.

The app starts its own localhost telemetry receiver when no other ABM
receiver owns it. Closing this standalone window also closes the receiver it
started. The game is never closed. If another service already provides the
receiver, the app reuses it instead.

Windows uses per-user local named pipes for commands and IPv4 loopback UDP port
44444 for game telemetry. No inbound internet/LAN access is required. Do not
open/forward this port on a router. Settings use Qt's per-user application config
location with the `abm` application ID. The legacy game extension ID remains
`beamdeck`; see the architecture guide for protocol compatibility details.

## Building

Use a Windows x64 MSVC developer shell with Qt 6.8.3 or newer (Desktop MSVC 2022
x64), Python 3 for CLI tests, and vcpkg. Install the archive reader:

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
& "$env:VCPKG_ROOT/vcpkg.exe" install libarchive:x64-windows
./bin/build-windows.ps1 -Test
```

`qmake`, `nmake` and `windeployqt` must be on PATH. The script builds the app,
runs offscreen Qt tests, deploys the Qt/archive DLLs into `dist/ABM`, and
tests the packaged CLI and standalone receiver lifecycle with isolated settings.
The `.github/workflows/windows.yml` workflow uploads the folder as an artifact.
It does not publish a release or require a signing key.

The portable folder is not code-signed yet, so Windows SmartScreen may warn.
Do not disable Windows security globally. Public releases should add signing,
third-party license notices and testing on a clean Windows machine before wider
distribution. Never include proprietary vehicle assets in the ZIP: previews are
read from the user's own game installation.

## Known boundaries

- Native Windows process detection and Steam lookup are implemented but need
  real-machine verification, including non-default Steam libraries.
- A launcher starting successfully is not a successful BeamMP server connection.
- An acknowledgement for spawning means submitted, not guaranteed acceptance.
- Custom configurations without pictures remain selectable with a clearly
  labeled model-preview fallback or "No preview available".
- Theme/language support falls back without Omarchy; there is no Windows bar
  plugin, background Windows service or automatic updater in this preview.
