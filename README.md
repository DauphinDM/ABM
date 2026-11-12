![ABM Drive page with labeled sample data](docs/screenshot.png)

# ABM — Advanced BeamNG.Drive Manager

A native, local-first manager for **BeamNG.drive and BeamMP**: launch the game,
watch live telemetry, browse your garage, and experiment with sandbox tuning.
Built with C++ and Qt Quick, with an optional Omarchy bar widget.

**Latest preview: [0.13.0](https://github.com/DauphinDM/ABM/releases/tag/v0.13.0).**
Public, MIT-licensed source with Linux and Windows downloads. This is an
independent project, not affiliated with BeamNG or BeamMP. Gameplay integration
and Windows desktop support remain experimental; passing CI is not live-game certification.

**New in 0.13.0:** [ABM Mobile](https://github.com/DauphinDM/ABM-Mobile)
and a fifth **Connect** screen for opt-in encrypted phone control over private Wi-Fi.
See [pairing, requirements and limits](docs/mobile-companion.md).
This release also offers [opt-in update checks and verified downloads](docs/updates.md); installation remains explicit and platform-specific.

## What it does

| Page | Features |
| --- | --- |
| **Drive** | Speed/RPM gauge, gear, fuel, speed history, coolant, boost, warnings |
| **Car** | Power entry, engine strength/protection, gravity, grip/brakes/steering, manual gears, ESC, bounce start/stop, experimental width/length/height, repair/fire/tires (safe extra gears and own-car multiplayer resizing require bridge 0.6.2; resizing is not synchronized to peers) |
| **Garage** | Combine vehicle type, stock/mod/custom content, favorites and nine metadata filters; sort models/configs, preview photos, confirm spawn/replace |
| **Session** | Trip time/distance, top/average speed, FPS history, searchable multiplayer players, owned vehicles |

Diagnostics adds temperature, pedals, steering, G forces, frame times and timing
estimates where the game supplies them. Missing readings remain unavailable,
not invented. Photos are read from your own installation, not downloaded.

The dashboard uses calm dark surfaces, larger instruments and photo cards, and
the live Omarchy accent. Tabs become a sidebar on wide windows; Garage filters
open separately so the vehicle grid stays usable. Theme-aware square corners,
plain text and keyboard navigation remain. English is included. The standalone
app works without Omarchy; Windows support remains experimental.

The Car workshop groups related controls into **Engine & protection**,
**Handling & gearbox**, **World & recovery**, and **Body · experimental**.
Vehicle capabilities, movement, invalid inputs and server policy disable
inapplicable controls. Install the updated bridge files and restart BeamNG
when ready to receive capability data. New live-power, actual-gear readback and
full-reset controls require bridge 0.9.0 or newer; see the [foundation guide](docs/workshop-foundation.md).
Bridge 0.10.0 adds supported combustion-engine RPM limits, friction-clutch
protection and same-vehicle reset recipes. See [reset persistence and RPM](docs/reset-persistence.md)
for compatibility and safety limits. These features are experimental, not whole-car invulnerability.

## Documentation

See the [changelog](CHANGELOG.md) for features, limits and validation status.

- [Installation and migration](docs/installation.md)
- [User guide, tuning behavior and shortcuts](docs/user-guide.md)
- [CLI reference](docs/cli.md)
- [Architecture and compatibility](docs/architecture.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Windows build and limitations](docs/windows.md)
- [Android/iOS companions and private-Wi-Fi pairing (development)](docs/mobile-companion.md)
- [Portable Linux: AppImage and bundled archive](docs/portable-linux.md)
- [Sandbox feature research and roadmap](docs/sandbox-research.md)
- [Official-server allowlist, cleanup and limitations](docs/server-policy.md)
- [Development, tests and releases](docs/development.md)
- [Visual audit](docs/ui-audit.md)
- [Design-to-code review and preview reliability](docs/design-review.md)
- [Security and privacy](SECURITY.md)
- [Changelog](CHANGELOG.md)

## Quick start — Arch Linux / Omarchy

Install the release's `abm-0.13.0-1-x86_64.pkg.tar.zst` asset:

```sh
sudo pacman -U abm-0.13.0-1-x86_64.pkg.tar.zst
abm-enable --json
abm open
```

`abm-enable` is for **Omarchy integration**. On other Linux desktops use the
standalone app's **Options → Paths & bridge setup** instead.

Confirm your active BeamNG user folder (contains `beamng.log`), install the
bridge, then restart BeamNG when convenient. The app never restarts an active
game automatically. Installing ABM does not install BeamNG or BeamMP.

On Omarchy, open **ABM** in the launcher. Left-click its bar widget to start
BeamNG when closed or show quick stats when running; right-click opens the
dashboard. An existing dashboard is focused rather than duplicated.

### Build from source

Requirements: a C++17 compiler, make, Qt 6 (base/declarative/Quick Controls,
Wayland and SVG runtime support), libarchive, and qmake6. The full test suite also
requires Python 3, Node.js and LuaJIT. Linux build scripts use `flock`.

```sh
gh repo clone DauphinDM/ABM
cd ABM
./bin/build
./bin/test
./build/abm open --demo
./bin/install
```

Other Linux desktops can use the [AppImage or portable archive](docs/portable-linux.md).
Windows users can extract the [standalone ZIP](docs/windows.md); keep its DLLs beside `ABM.exe`.

## Tuning is deliberately a sandbox

Use **Apply tuning** after moving sliders or changing beam protection.
Gravity preset buttons apply immediately and affect **the whole world**, not
only your vehicle. Power scales engine torque curves, not a guaranteed measured
horsepower value. Extra torque can break the drivetrain. Beam protection raises
beam deformation/strength limits; it does not protect every engine or tire.

The displayed applied values are the last accepted request, not verified physics
readback. Tuning re-applies periodically to the same vehicle, so an ordinary
vehicle reset is not a reliable way to clear it. Apply 100% power, Earth gravity
and protection off to restore stock settings. See the [user guide](docs/user-guide.md).

**Use only where allowed.** This is not an anti-cheat bypass. Multiplayer servers
may reject or override actions. Effects of the new tuning controls still need
live verification after loading the bridge; mocks and automated tests are not
a substitute for real-game testing.

Development 0.7 requires a verified server identity before multiplayer changes:
only the frozen regular-official roster is approved; official events and new or
mismatched official servers are blocked. Verified non-official servers have no
extra ABM restriction. [Policy details and important limitations](docs/server-policy.md).

## Local by design

The bridge sends bounded JSON to `127.0.0.1:44444`. The receiver fetches BeamMP's
public HTTPS server directory for official-server classification; it uploads no
telemetry or connected-server identity. There is no ABM cloud service,
account login, remote console, arbitrary Lua-execution endpoint or automatic
updater. The local game installation and BeamMP-managed files are not edited.
ABM installs four bridge files in your **game user folder** and backs up changed
integration files. See [security boundaries](SECURITY.md).

## Preview status

- Linux: built, packaged, tested with isolated UDP/local sockets and real
  offscreen QML; Omarchy widget visually inspected on a live desktop.
- New tuning: generated vehicle Lua is executed in tests, including repeated
  application and restoring stock beam limits. Actual game effects remain
  a live-testing task.
- Windows: build/deployment workflow provided; clean-machine gameplay and
  launcher behavior are not yet certified. Do not treat it as stable support.
- No complete ECU/parts editor, persistent named tune profiles, cloud sync or
  online mod downloads.

The name is ABM throughout the application. A few **legacy protocol and bridge
identifiers** intentionally remain for compatibility; [the architecture guide](docs/architecture.md)
lists them. The executable, package, service, desktop ID and widget use `abm`.

## License

MIT; see [LICENSE](LICENSE). Third-party applications, libraries and game assets
retain their own licenses. ABM does not redistribute BeamNG vehicle archives.
