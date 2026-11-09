# Troubleshooting ABM

## Start with real status

```sh
abm status --json
abm launch-status --json
systemctl --user status abm.service
journalctl --user -u abm.service -n 50 --no-pager
```

The last two commands apply to Omarchy/service installations. A disconnected
feed does not prove a game crash. Never use demo screenshots as evidence that
the actual game is connected.

## Game runs but ABM waits for the bridge

1. Verify the active game **user folder**, not merely the Steam install path.
2. Install the bridge through Options/setup or `abm-enable`.
3. Restart BeamNG when convenient or reload the extension in **GE Lua**:
   `extensions.unload("beamdeck"); extensions.load("beamdeck")`.
4. Check `beamng.log` for extension/LuaSocket errors.
5. Ensure only one receiver owns localhost UDP 44444. Stop an old development
   service/window rather than starting competing receiver instances.

## Car controls are unavailable or do not change the car

Car controls need bridge **0.4.0**, a loaded active vehicle and ownership.
The bridge installed on disk is not necessarily the one loaded in the running
game. New controls won't load until reload/restart.

Apply tuning after adjustments; presets apply immediately. Displayed values are
requests, not actual dyno readback. Some electric/modded engines may not expose
the supported torque curve. Servers can reject/override requests. Beam protection
does not prevent every mechanical failure. Restore 100%/Earth/protection off
explicitly; see [tuning behavior](user-guide.md).

## Blank previews or missing configurations

Confirm the installation path in setup. Custom configurations without their own
image intentionally use a labeled model image or no-preview tile. Refresh the
catalog using Garage's menu after installing new vehicle content. Do not copy
proprietary vehicle archives into this repository or into bug attachments.

## Widget missing, stale, or too small

```sh
omarchy plugin validate ~/.config/omarchy/plugins/snobbledev.abm
omarchy-shell snobbledev.abm state
journalctl --user -t omarchy-shell -n 80 --no-pager
omarchy restart shell
```

Use the restart after confirming the current plugin files are installed. A
rescan does not reliably replace already-instantiated widget QML. Multiple
monitor instances can produce a duplicate IPC-handler warning; this alone is
not a widget load failure. `Cannot assign` / `Failed to load` errors are relevant.

## BeamMP does not start or connect

Install BeamMP separately and select its real executable. ABM starts it with
`--no-launch` from the launcher's own directory. A running launcher is not a
connected server. The bridge can enable an installed mod but cannot download
missing BeamMP content or bypass server restrictions.

## Reporting a problem privately

Use the private repository's Issues tab. Include ABM/bridge/game version,
platform, installation method, steps, expected/actual behavior and redacted
diagnostics. Avoid full home paths, credentials, chat logs, usernames/server
addresses, and proprietary archives. Security-sensitive reports should follow
[SECURITY.md](../SECURITY.md).
