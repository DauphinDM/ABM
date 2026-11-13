# Portable Linux builds

ABM's desktop app doesn't need Omarchy, Hyprland, pacman, or systemd. The Omarchy **bar widget** does need the Omarchy shell. On other desktops, the app runs its own receiver while its window is open. Steam and BeamNG are separate installations. ABM doesn't bundle or download them.

## AppImage: the preferred portable format

Download `ABM-<version>-x86_64.AppImage` from [GitHub Releases](https://github.com/DauphinDM/ABM/releases). Check its matching entry in `SHA256SUMS`, then run:

```sh
chmod +x ABM-<version>-x86_64.AppImage
./ABM-<version>-x86_64.AppImage open
```

Qt, QML controls, archive support, and desktop platform plugins are bundled. You don't need administrator installation. Use Paths & bridge setup inside ABM to select the BeamNG user folder and install the bundled game extension. The AppImage doesn't install an Omarchy plugin or a user service automatically.

If FUSE isn't available, use the companion `.tar.gz` archive and run `ABM.AppDir/AppRun open`, or use:

```sh
./ABM-<version>-x86_64.AppImage --appimage-extract-and-run open
```

Keep the whole extracted directory together. You can move the AppImage. Preferences stay in the normal per-user ABM config directory.

## What "compatible" means here

This is a compatibility contract, not an "every distro" promise:

- x86_64, glibc-based Linux desktop, X11 or Wayland. No ARM build yet.
- Built on Ubuntu 22.04 with Qt 6.8.3. The intended minimum is glibc 2.35. The actual artifact's linked Qt and libraries have to meet that baseline in CI.
- CI smoke matrix: Ubuntu 22.04, Debian 12, Fedora 43, and current Arch. Containers install host graphics and font libraries, not a system Qt runtime. A passing offscreen startup is **not** proof that every GPU, compositor, or gameplay feature works.
- Alpine/musl, older glibc systems, and missing host graphics support are not covered. An AppImage can't turn a glibc binary into a musl or ARM binary.
- If Wayland startup fails, try `QT_QPA_PLATFORM=xcb` with an X11 or XWayland session. Actual graphical compatibility still depends on your drivers.
- The first recipe is experimental until its build and distro smoke jobs pass. The published private v0.4.0 doesn't include these portable artifacts.

AppImage avoids Flatpak's filesystem and process sandbox complications when installing a BeamNG extension or launching host Steam and BeamMP. Flatpak is a possible later option, not a second untested packaging claim.

## Build

On Ubuntu 22.04 with Qt 6.8.3's `qmake` on PATH, plus libarchive development files, build-essential, curl, desktop-file-utils, and patchelf:

```sh
QMAKE=qmake bash bin/build-appimage
```

See `.github/workflows/appimage.yml` for the full dependency list. Don't build the portable release on rolling Arch. Newer glibc requirements would undermine compatibility. Tool downloads are SHA256-pinned and refuse to build if upstream replaces a continuous build. Updating a tool requires a new hash.

The unpacked AppDir uses shared libraries, which you can replace for debugging or LGPL relinking. See the bundled licenses and [THIRD-PARTY.md](THIRD-PARTY.md).

## Sources

- [AppImage concepts and oldest-system build guidance](https://docs.appimage.org/introduction/concepts.html)
- [Qt deployment plugin: Qt 6, QML, Wayland](https://github.com/linuxdeploy/linuxdeploy-plugin-qt)
- [Qt 6.8 Linux supported configurations and the binary glibc caveat](https://doc.qt.io/qt-6.8/linux.html)
