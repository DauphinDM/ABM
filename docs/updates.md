# App updates (development)

**Connect → App updates** gives you opt-in release checks on Linux, Windows, Android, and native iOS. It's included from PC 0.13.0. Older installations need one manual update to get it.

All options start off. You can enable checks at startup or daily, include preview releases, and optionally allow automatic verified downloads on PC and Android. Checks contact GitHub, which sees your IP address. They don't send pairing links, saved-device tokens, telemetry, or signing credentials. Downloads never interrupt your game or install silently.

## Installing on each platform

- **Linux:** ABM downloads a verified AppImage. You open it yourself, make it executable, and run it after closing ABM. Arch and Omarchy package-managed installs must update through the package manager. ABM never replaces `/usr/bin` or runs administrator commands.
- **Windows:** ABM downloads a verified ZIP. You open it, close ABM, and extract it into a new folder. Keep the DLLs and licenses together. There's no unattended extraction or self-replacement.
- **Android:** ABM downloads a verified APK and hands it to the OS installer only when you ask. Android needs install-source permission, your confirmation, the same app ID and signing certificate, and a suitable version code. A signature mismatch never triggers an automatic uninstall or data loss.
- **iOS:** automatic checks and a link to the official release page. Sideloaded IPAs still need local re-signing. TestFlight and App Store updates can be used once those channels exist. Neither is configured now. ABM collects no Apple credentials and has no in-app IPA installer.

Automatic **checking and downloading** works. Installation is still explicit, controlled by the OS, or handled by your package manager. There is no published mobile release yet. CI artifacts are not a release feed.

## Trust and limits

ABM only considers published releases from `DauphinDM/ABM` or `DauphinDM/ABM-Mobile`. Drafts are excluded, and previews are excluded by default. Tags must be strictly newer three-part versions: `vX.Y.Z`, optionally with `-preview.N`, `-beta.N`, or `-rc.N`. Comparison uses the three numeric components, not progress between previews of the same version. Same-version build numbers and development commits are not updates.

PC and Android packages need an exact official repository and tag URL, a safe platform and version filename, a size from 1 to 512 MiB, and a GitHub API `sha256:` digest. Responses are capped at 1 MiB. Transfers stream into atomic files and are checked for exact length and hash before acceptance. Redirects are limited to GitHub HTTPS asset hosts. API redirects, URL credentials, and plain HTTP are refused. The file is hashed again before it goes to the OS. If the digest is missing, downloading is disabled, but the release-page link still works.

These hashes show that a file matches the release metadata. They don't independently authenticate the publisher. If GitHub or the repository is compromised, that's still a supply-chain risk. Windows and AppImage files are not code-signed. Android still enforces its signing certificate. iOS needs valid provisioning.

## Publishing updates

1. Bump the app's three-part version and the mobile build and version code, then pass platform tests.
2. Publish a matching versioned release in the official repository, not just an Actions artifact.
3. Upload `ABM-X.Y.Z-x86_64.AppImage`, `ABM-X.Y.Z-Windows-x64.zip`, or `ABM-Mobile-X.Y.Z-Android-arm64.apk`. Verify the API SHA-256 digests, include checksums and licenses, and keep assets immutable. Android `-debug.apk` files are excluded on purpose.
4. Keep Android's release signing key. The mobile workflow can optionally sign release APKs with the repository secrets `ABM_ANDROID_KEYSTORE_B64`, `ABM_ANDROID_STORE_PASSWORD`, `ABM_ANDROID_KEY_ALIAS`, and `ABM_ANDROID_KEY_PASSWORD` on trusted push or manual builds. Without them, only development APKs are produced. This optional path needs maintainer setup and hasn't been tried with a real key. These secrets are separate from iPhone P12 files. Never commit keys or post them in issues or logs.

No release, store account, or signing key is created automatically.
