# ABM command reference

Run `abm --help` for the built-in summary. Headless commands do not need a
display server. JSON output is written to stdout; diagnostics may use stderr.

| Command | Purpose |
| --- | --- |
| `abm open` | Open/focus the dashboard; reuse a receiver or start an app-owned one |
| `abm open --tab drive\|car\|garage\|session` | Select a page, including on an existing dashboard |
| `abm serve` | Run the local telemetry receiver in the foreground |
| `abm status --json` | Read current state; exit 0 connected, 1 disconnected/error |
| `abm launch-status --json` | Inspect game/BeamMP process state and launch configuration |
| `abm launch --auto --json` | Start BeamNG if closed, otherwise start BeamMP with `--no-launch` |
| `abm setup --user-folder PATH [--game-folder PATH] [--launcher PATH] --json` | Configure paths and install the bridge |
| `abm preferences --json` | Read unit preference/favorites |
| `abm preferences --units kmh\|mph --json` | Set speed/distance units |
| `abm preferences --favorite MODEL[/CONFIG] --json` | Toggle a favorite identifier |
| `abm skill install --json` | Install the bundled agent guide in `~/.agents/skills/abm/` |

The setup command expects a real user folder containing `beamng.log`. It will
not create a fake game profile. The skill installer refuses to overwrite an
unrelated existing guide. There is no CLI that accepts arbitrary Lua code.

## Demo and screenshot mode

```sh
abm open --demo --tab car
QT_QUICK_BACKEND=software abm open --demo --tab garage --pick etk800 --screenshot /tmp/abm-configs.png
QT_QUICK_BACKEND=software abm open --demo --dev-open diagnostics --screenshot /tmp/abm-diagnostics.png
```

`--screenshot` requires `--demo` and an output path. It renders offscreen with
sample data and exits. `--pick` and `--dev-open` only affect demo captures.
See [visual audit targets](ui-audit.md). Demo actions never control the real game.

## Isolation and compatibility environment variables

The first release retains these legacy names for existing integrations/tests:

| Variable | Meaning |
| --- | --- |
| `BEAMSIDE_RUNTIME_DIR` | Override state/socket directory |
| `BEAMSIDE_CONFIG_DIR` | Override preference/integration directory |
| `BEAMSIDE_BINARY` | Test scripts' executable override; not an app runtime setting |
| `VCPKG_ROOT` | Windows archive-library include/library/deployment root |

Tests also use `--receiver-port PORT` to avoid the real game's UDP port.
Do not change production port 44444 without coordinating the bridge, which
uses a fixed receiver destination.
