# Portable binary dependencies

ABM's source is MIT-licensed. Its dynamically linked dependencies keep their own licenses. ABM's license doesn't replace them.

- **Qt 6.8.3:** LGPL-3.0 or GPL, as applicable to Qt Base, Declarative, SVG, and Wayland. This app uses shared libraries, not a statically linked Qt. [License terms](https://www.qt.io/licensing/open-source-lgpl-obligations). [Exact Qt 6.8.3 source archive](https://download.qt.io/archive/qt/6.8/6.8.3/single/).
- **libarchive and its dependency libraries:** BSD-style and other upstream licenses. Ubuntu dependency copyright notices are bundled under `usr/share/doc/abm/dependencies/`, with source and package information. Windows builds include vcpkg dependency notices under `licenses/<package>/copyright`. Matching sources are available through vcpkg's package ports and upstream links. [libarchive source](https://github.com/libarchive/libarchive).
- **OpenSSL 3:** Apache-2.0. Used through shared libraries for local certificate generation and Qt's TLS backend; ABM doesn't run or require the OpenSSL command-line tool. Its dependency notices ship with the Windows and portable Linux bundles. [License and source](https://www.openssl.org/source/).
- The LGPL-3 and GPL-3 license texts ship with the AppDir. You can get Ubuntu library sources with `apt-get source <package>` using the matching Ubuntu 22.04 source repositories. The notices name their upstreams. Windows ZIPs also include the LGPL-3 and GPL-3 texts under `licenses/`.

Extract the AppImage with `--appimage-extract`, or use the companion archive, to inspect or replace shared libraries. No application signature or loader policy stops you from running a modified library. Keep dependency notices when you redistribute.

No proprietary BeamNG files, Steam, or BeamMP launcher are included.
