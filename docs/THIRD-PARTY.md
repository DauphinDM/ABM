# Portable binary dependencies

ABM source is MIT-licensed. Its dynamically linked dependencies have their
own licenses; ABM's license does not replace them.

- **Qt 6.8.3**: LGPL-3.0 / GPL licensing as applicable to Qt Base, Declarative,
  SVG and Wayland. This app uses shared libraries, not a statically linked Qt.
  [License terms](https://www.qt.io/licensing/open-source-lgpl-obligations).
  [Exact Qt 6.8.3 source archive](https://download.qt.io/archive/qt/6.8/6.8.3/single/).
- **libarchive** and its dependency libraries: BSD-style and other upstream
  licenses. Ubuntu dependency copyright notices are bundled under
  `usr/share/doc/abm/dependencies/` with source/package information.
  [libarchive source](https://github.com/libarchive/libarchive).
- The LGPL-3 and GPL-3 license texts accompany the AppDir. Ubuntu-provided
  library sources can be obtained with `apt-get source <package>` using the
  matching Ubuntu 22.04 source repositories; notices name their upstreams.

Extract the AppImage (`--appimage-extract`) or use the companion archive to
inspect/replace shared libraries. No application signature or loader policy
prevents running a modified library. Keep dependency notices when redistributing.
No proprietary BeamNG files, Steam, or BeamMP launcher are included.
