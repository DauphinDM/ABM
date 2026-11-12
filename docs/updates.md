# App updates (development)

**Connect → App updates** provides opt-in release checks on Linux, Windows, Android and native iOS. This is not in the published PC 0.12.0 binary; existing installations need one manual update to obtain it.

All options default off. Enable startup/daily checks, preview releases, and optionally automatic verified downloads on PC/Android. Checks contact GitHub, exposing your IP, but send no pairing links, saved-device tokens, telemetry or signing credentials. Downloads never interrupt your game or install silently.

## Platform installation

- **Linux:** verified AppImage download; explicitly open it, make it executable and run after closing ABM. Arch/Omarchy package-managed installations must update through the package manager. No `/usr/bin` replacement or administrator commands.
- **Windows:** verified ZIP download; open it, close ABM and extract into a new folder. Keep DLLs/licenses together. No unattended extraction or self-replacement.
- **Android:** verified APK download with an explicit OS-installer action. Android requires install-source permission, confirmation, the same app ID/signing certificate, and an appropriate version code. Signature mismatch never triggers automatic uninstall/data loss.
- **iOS:** automatic checks and an official release-page link. Sideloaded IPAs still need local re-signing. TestFlight/App Store updates can be used once those channels exist; neither is currently configured. No Apple credentials are collected and there is no in-app IPA installer.

Automatic **checking/downloading** is implemented; installation remains explicit, OS-controlled or package-managed. There is no published mobile release yet. CI artifacts are not a release feed.

## Trust and limits

Only published releases from `DauphinDM/ABM` or `DauphinDM/ABM-Mobile` are considered. Drafts and, by default, previews are excluded. Tags must be strictly newer three-part versions: `vX.Y.Z`, optionally `-preview.N`, `-beta.N` or `-rc.N`. Comparison uses the three numeric components, not progression between previews of the same version. Same-version build numbers and development commits are not updates.

PC/Android packages require an exact official repository/tag URL, safe platform/version filename, size 1–512 MiB and GitHub API `sha256:` digest. Responses are limited to 1 MiB; transfers are streamed into atomic files and checked for exact length/hash before acceptance. Redirects are bounded to GitHub HTTPS asset hosts; API redirects, URL credentials and HTTP are refused. The file is rehashed before OS handoff. Missing digest disables downloading but not the release-page link.

These hashes establish correspondence to release metadata, not independent publisher authentication: compromise of GitHub/the repository remains a supply-chain risk. Windows/AppImage files are not code-signed. Android still enforces its signing certificate; iOS requires valid provisioning.

## Publishing updates

1. Bump the app's three-part version and mobile build/version code, then pass platform tests.
2. Publish a matching versioned release in the official repository, not just an Actions artifact.
3. Upload `ABM-X.Y.Z-x86_64.AppImage`, `ABM-X.Y.Z-Windows-x64.zip`, or `ABM-Mobile-X.Y.Z-Android-arm64.apk`. Verify API SHA-256 digests, include checksums/licenses and keep assets immutable. Android `-debug.apk` files are deliberately excluded.
4. Preserve Android's release signing key. The mobile workflow optionally signs release APKs using repository secrets `ABM_ANDROID_KEYSTORE_B64`, `ABM_ANDROID_STORE_PASSWORD`, `ABM_ANDROID_KEY_ALIAS`, `ABM_ANDROID_KEY_PASSWORD` on trusted push/manual builds. Without them only development APKs are produced. This optional path needs maintainer configuration and has not been exercised without a real key. These secrets are unrelated to iPhone P12 files; never commit keys or post them in issues/logs.

No release, store account or signing key is created automatically.
