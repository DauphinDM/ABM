# Installing ABM

## Supported paths

| Platform | Installation | Status |
| --- | --- | --- |
| Arch / Omarchy, x86_64 | Private release package or `bin/install` | Primary development target |
| Other Linux desktops | Build and run `build/abm open` | Standalone mode; no Omarchy widget |
| Windows x64 | Qt/MSVC build or authorized CI artifact | Experimental; see [Windows guide](windows.md) |
| Linux aarch64 | PKGBUILD declares architecture | No prebuilt asset or hardware validation |

BeamNG and (optionally) BeamMP must already be installed. ABM is not a game or
launcher downloader. A running Steam installation is needed to start BeamNG
through its Steam URI.

## Arch package

Download from the private repository with an authorized GitHub account:

```sh
gh release download v0.4.0 --repo DauphinDM/ABM --pattern '*.pkg.tar.zst'
gh release download v0.4.0 --repo DauphinDM/ABM --pattern SHA256SUMS
sha256sum -c SHA256SUMS
sudo pacman -U abm-0.4.0-1-x86_64.pkg.tar.zst
```

If only a subset of assets is downloaded, the checksum command may report
missing files listed in the manifest. Verify the downloaded package's matching
entry before installing. Checksums detect corruption; they are not signatures.

Package dependencies include Qt 6 base, declarative, Wayland, SVG and libarchive.
Build with `./bin/install` to use the normal makepkg dependency/install prompts.

## Omarchy integration

```sh
abm-enable --json
# For a custom user folder:
abm-enable --user-folder /path/to/active/BeamNG/user-folder --json
```

This validates/copies the plugin, installs the game-user-folder bridge, starts
`abm.service`, and enables `snobbledev.abm` in the bar's `centerEnd` section.
It does not modify `/usr/share/omarchy/`.

If an old widget instance remains after upgrading, use `omarchy restart shell`.
The shell restart temporarily reloads the whole desktop bar.

## Standalone setup

Run `abm open`, open **Drive → Options → Paths & bridge setup**, and set:

1. **User folder:** the active folder containing `beamng.log`, not the game install.
2. **Game folder:** the BeamNG installation directory, used for previews.
3. **BeamMP launcher:** its executable, if multiplayer launching is wanted.

Choose **Save & install bridge**. The standalone app owns a local receiver if
none is available; it stops that receiver when the window closes. It does not
install a background service on non-Omarchy platforms.

## Loading the bridge

Restart the game when convenient, or open the game console, choose **GE Lua**,
and run:

```lua
extensions.unload("beamdeck"); extensions.load("beamdeck")
```

`beamdeck` is the intentionally retained extension ID. The files are:

```text
<user folder>/lua/ge/extensions/beamdeck.lua
<user folder>/scripts/beamdeck/modScript.lua
```

The user folder defaults to `~/.local/share/BeamNG/BeamNG.drive/current` on
Linux. Do not assume it matches a cloud profile or a relocated installation.
Garage actions accept bridge 0.3/0.4; Car actions need 0.4.

## Migrating a previous development installation

The `abm` Arch package conflicts with/replaces the old `beamdeck` package.
The enable helper stops/disables the legacy enabled receiver, disables the old
widget, and copies `preferences.ini`/`integration.ini` to `~/.config/abm/` only
when the corresponding destination file does not already exist. Legacy user
files are preserved, not recursively removed. Only one receiver can own UDP
44444; close an old standalone app before enabling the replacement.

Custom compositor rules referring to the old application ID will not match
`abm`. Update your own rule explicitly if desired; ABM does not rewrite it.

## Disable or remove

```sh
omarchy plugin disable snobbledev.abm
systemctl --user disable --now abm.service
sudo pacman -R abm
```

Unload the bridge in GE Lua with `extensions.unload("beamdeck")`. To stop
future loading, remove only the two ABM-owned bridge files listed above after
checking ownership/backups. Do not delete your BeamNG user folder or BeamMP
managed files. Preferences and user plugin copies remain until you remove them
explicitly.
