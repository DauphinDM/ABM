# ABM command reference

Run `abm --help` for the built-in summary. Headless commands don't need a display server. JSON output goes to stdout. Diagnostics may go to stderr.

| Command | What it does |
| --- | --- |
| `abm open` | Open or focus the dashboard. Reuse a receiver or start one owned by the app. |
| `abm open --tab drive\|car\|garage\|session` | Select a page, including on an existing dashboard. |
| `abm serve` | Run the local telemetry receiver in the foreground. |
| `abm status --json` | Read the current state. Exit 0 means connected; 1 means disconnected or an error. |
| `abm launch-status --json` | Check game and BeamMP process state and launch configuration. |
| `abm launch --auto --json` | Start BeamNG if it's closed, otherwise start BeamMP with `--no-launch`. |
| `abm setup --user-folder PATH [--game-folder PATH] [--launcher PATH] --json` | Set paths and install the bridge. |
| `abm preferences --json` | Read unit preference and favorites. |
| `abm preferences --units kmh\|mph --json` | Set speed and distance units. |
| `abm preferences --favorite MODEL[/CONFIG] --json` | Toggle a favorite identifier. |
| `abm skill install --json` | Install the bundled agent guide in `~/.agents/skills/abm/`. |

The setup command expects a real user folder containing `beamng.log`. It won't create a fake game profile. The skill installer refuses to overwrite an unrelated existing guide. No CLI accepts arbitrary Lua code.

## Demo and screenshot mode

```sh
abm open --demo --tab car
QT_QUICK_BACKEND=software abm open --demo --tab garage --pick etk800 --screenshot /tmp/abm-configs.png
QT_QUICK_BACKEND=software abm open --demo --dev-open diagnostics --screenshot /tmp/abm-diagnostics.png
```

`--screenshot` needs `--demo` and an output path. It renders offscreen with sample data and exits. `--pick` and `--dev-open` only affect demo captures. See [visual audit targets](ui-audit.md). Demo actions never control the real game.

## Isolation and compatibility environment variables

The first release keeps these legacy names for existing integrations and tests:

| Variable | Meaning |
| --- | --- |
| `BEAMSIDE_RUNTIME_DIR` | Override the state and socket directory. |
| `BEAMSIDE_CONFIG_DIR` | Override the preference and integration directory. |
| `BEAMSIDE_BINARY` | Executable override for test scripts. Not an app runtime setting. |
| `VCPKG_ROOT` | Windows archive-library include, library, and deployment root. |

Tests also use `--receiver-port PORT` to avoid the real game's UDP port. Don't change production port 44444 without coordinating the bridge, which sends to a fixed receiver address.
