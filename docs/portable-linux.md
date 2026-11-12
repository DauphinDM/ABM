# Portable Linux builds

ABM's desktop app does not require Omarchy, Hyprland, pacman or systemd.
The Omarchy **bar widget** does require the Omarchy shell. On other desktops,
the app starts its own receiver while its window is open. Steam and BeamNG
are separate host installations, not bundled or downloaded by ABM.

## AppImage — preferred portable format

Download `ABM-<version>-x86_64.AppImage` from
[GitHub Releases](https://github.com/DauphinDM/ABM/releases). Check its matching
entry in `SHA256SUMS`, then:

```sh
chmod +x ABM-<version>-x86_64.AppImage
./ABM-<version>-x86_64.AppImage open
```

Qt, QML controls, archive support and desktop platform plugins are bundled.
No administrator installation is needed. Use Paths & bridge setup inside ABM
to select the BeamNG user folder and install the bundled game extension.
No Omarchy plugin or user service is installed automatically by the AppImage.

If FUSE is unavailable, either use the companion `.tar.gz` archive and run
`ABM.AppDir/AppRun open`, or use:

```sh
./ABM-<version>-x86_64.AppImage --appimage-extract-and-run open
```

Keep the entire extracted directory together. Moving the AppImage is fine;
preferences remain in the normal per-user ABM config directory.

## Compatibility contract, not an "every distro" promise

- x86_64, glibc-based Linux desktop, X11 or Wayland. No ARM build yet.
- Built on Ubuntu 22.04 with Qt 6.8.3. The intended minimum is glibc 2.35;
  the actual artifact's linked Qt/libraries must meet that baseline in CI.
- CI smoke matrix: Ubuntu 22.04, Debian 12, Fedora 43 and current Arch.
  Containers install host graphics/font libraries, not a system Qt runtime.
  A passing offscreen startup is **not** proof of every GPU/compositor or gameplay.
- Alpine/musl, older glibc systems and missing host graphics support are not
  covered. AppImage cannot turn a glibc binary into a musl or ARM binary.
- If Wayland startup fails, try `QT_QPA_PLATFORM=xcb` with an X11/XWayland
  session. Actual graphical compatibility still depends on the host drivers.
- The first recipe is experimental until its build and distro smoke jobs pass.
  Published private v0.4.0 does not include these portable artifacts.

AppImage avoids Flatpak's filesystem/process sandbox complications when
installing a BeamNG extension or launching host Steam/BeamMP. Flatpak is a
possible later option, not a second untested packaging claim.

## Build

On Ubuntu 22.04 with Qt 6.8.3's `qmake` on PATH, libarchive development files,
build-essential, curl, desktop-file-utils and patchelf:

```sh
QMAKE=qmake bash bin/build-appimage
```

See `.github/workflows/appimage.yml` for the full dependency list. Do not
build the portable release on rolling Arch: newer glibc requirements would
undermine compatibility. Tool downloads are SHA256-pinned and fail closed
if upstream replaces a continuous build; updating a tool requires a new hash.

The unpacked AppDir uses shared libraries, which can be replaced for debugging
or LGPL relinking. See bundled licenses and [THIRD-PARTY.md](THIRD-PARTY.md).

## Sources

- [AppImage concepts and oldest-system build guidance](https://docs.appimage.org/introduction/concepts.html)
- [Qt deployment plugin: Qt 6, QML, Wayland](https://github.com/linuxdeploy/linuxdeploy-plugin-qt)
- [Qt 6.8 Linux supported configurations and binary glibc caveat](https://doc.qt.io/qt-6.8/linux.html)
