# Installing ABM

## Supported setups

| Platform | How to install | Status |
| --- | --- | --- |
| Arch / Omarchy, x86_64 | Release package or `bin/install` | Main development target |
| Other Linux desktops | [AppImage or bundled archive](portable-linux.md), or build from source | Portable CI recipe is experimental. Standalone mode only, no Omarchy widget. |
| Windows x64 | Standalone release ZIP or a Qt/MSVC build | Experimental. See the [Windows guide](windows.md). |
| Linux aarch64 | PKGBUILD declares the architecture | No prebuilt asset and no hardware validation |

You need BeamNG and, if you want multiplayer, BeamMP installed already. ABM is not a game or launcher downloader. Starting BeamNG through its Steam URI requires a running Steam installation.

## Arch package

Download the package from [GitHub Releases](https://github.com/DauphinDM/ABM/releases/tag/v0.13.0):

```sh
gh release download v0.13.0 --repo DauphinDM/ABM --pattern 'abm-*.pkg.tar.zst'
gh release download v0.13.0 --repo DauphinDM/ABM --pattern SHA256SUMS
sha256sum -c SHA256SUMS
sudo pacman -U abm-0.13.0-1-x86_64.pkg.tar.zst
```

If you download only some of the assets, the checksum command may complain about missing files listed in the manifest. Check the package's matching entry before installing. Checksums catch corruption. They are not signatures.

The package depends on Qt 6 base, declarative, Wayland, SVG, and libarchive. Running `./bin/install` uses the normal makepkg dependency and install prompts.

## Omarchy integration

```sh
abm-enable --json
# For a custom user folder:
abm-enable --user-folder /path/to/active/BeamNG/user-folder --json
```

This checks and copies the plugin, installs the bridge into your game user folder, starts `abm.service`, and enables `snobbledev.abm` in the bar's `centerEnd` section. It does not modify `/usr/share/omarchy/`.

If an old widget instance sticks around after an upgrade, run `omarchy restart shell`. That briefly reloads the whole desktop bar.

## Standalone setup

Run `abm open`, then open **Drive → Options → Paths & bridge setup** and fill in:

1. **User folder:** the active folder that contains `beamng.log`, not the game install folder.
2. **Game folder:** the BeamNG installation directory, used for previews.
3. **BeamMP launcher:** the launcher executable, if you want to start multiplayer from ABM.

Choose **Save & install bridge**. The standalone app runs its own local receiver when no other one is available, and stops that receiver when the window closes. It does not install a background service on non-Omarchy platforms.

## Loading the bridge

Restart the game when convenient, or open the game console, choose **GE Lua**, and run:

```lua
extensions.unload("beamdeck"); extensions.load("beamdeck")
```

`beamdeck` is the extension ID, kept on purpose for compatibility. The files are:

```text
<user folder>/lua/ge/extensions/beamdeck.lua
<user folder>/scripts/beamdeck/modScript.lua
<user folder>/lua/vehicle/extensions/abmTuning.lua
<user folder>/lua/ge/extensions/abm-official-servers.json
```

On Linux, the user folder defaults to `~/.local/share/BeamNG/BeamNG.drive/current`. Don't assume it matches a cloud profile or a relocated installation.

Garage actions work with bridge 0.3 through 0.6. Original Car actions need 0.4 or newer. Handling, gearbox, bounce, and uncapped power need 0.5 or newer. Engine durability needs 0.6. Multiplayer gameplay changes need bridge 0.7 and verified server permission. Restart the game after upgrading all four files. Reloading GE Lua by itself does not replace a vehicle extension that's already loaded in its separate VM.

## Moving from an older development install

The `abm` Arch package conflicts with and replaces the old `beamdeck` package. The enable helper stops and disables the legacy receiver, disables the old widget, and copies `preferences.ini` and `integration.ini` to `~/.config/abm/`, but only if the destination file doesn't already exist. Your legacy user files are left alone. They are not deleted recursively. Only one receiver can own UDP 44444, so close any old standalone app before enabling the replacement.

Custom compositor rules that refer to the old application ID won't match `abm`. Update them yourself if you want to keep them. ABM does not rewrite your compositor config.

## Disable or remove

```sh
omarchy plugin disable snobbledev.abm
systemctl --user disable --now abm.service
sudo pacman -R abm
```

To unload the bridge, run `extensions.unload("beamdeck")` in GE Lua. To stop it loading in the future, remove only the four ABM-owned bridge files listed above, after checking ownership and backups. Don't delete your BeamNG user folder or BeamMP-managed files. Preferences and copied plugin files stay until you remove them yourself.
