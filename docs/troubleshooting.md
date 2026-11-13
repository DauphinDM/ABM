# Troubleshooting ABM

## Start with real status

```sh
abm status --json
abm launch-status --json
systemctl --user status abm.service
journalctl --user -u abm.service -n 50 --no-pager
```

The last two commands only apply to Omarchy and service installs. A disconnected feed doesn't prove the game crashed. Never use demo screenshots as evidence that the real game is connected.

## The game runs, but ABM waits for the bridge

1. Check the active game **user folder**, not just the Steam install path.
2. Install the bridge through Options and setup, or run `abm-enable`.
3. Restart BeamNG when convenient, or reload the extension in **GE Lua**: `extensions.unload("beamdeck"); extensions.load("beamdeck")`.
4. Check `beamng.log` for extension and LuaSocket errors.
5. Make sure only one receiver owns localhost UDP 44444. Stop an old development service or window instead of starting competing receivers.

## Car controls are missing or don't change the car

Original Car controls need bridge **0.4.0**. New handling, gearbox, bounce, and uncapped power need **0.5.0**, plus a loaded active vehicle and ownership. Install the bridge again so both the GE and vehicle extensions are copied, then restart the game when convenient. A paused physics VM may time out. ABM doesn't report that as success.

The bridge on disk is not necessarily the one loaded in the running game. New controls won't load until you reload or restart.

Apply tuning after adjustments. Presets apply immediately. Displayed values are requests, not dyno readback. Some electric or modded engines may not expose the supported torque curve. Servers can reject or override requests. Beam protection does not prevent every mechanical failure. To restore normal behavior, set 100%, Earth gravity, and protection off yourself. See [tuning behavior](user-guide.md).

## Blank previews or missing configurations

Check the installation path in setup. Custom configurations without their own image use a labeled model image or a no-preview tile on purpose. Refresh the catalog from Garage's menu after installing new vehicle content. Don't copy proprietary vehicle archives into this repository or into bug attachments.

## The widget is missing, stale, or too small

```sh
omarchy plugin validate ~/.config/omarchy/plugins/snobbledev.abm
omarchy-shell snobbledev.abm state
journalctl --user -t omarchy-shell -n 80 --no-pager
omarchy restart shell
```

Restart the shell after confirming the current plugin files are installed. A rescan doesn't reliably replace widget QML that's already running. Multiple monitor instances can produce a duplicate IPC-handler warning. That warning by itself doesn't mean the widget failed to load. `Cannot assign` and `Failed to load` errors are the ones to look for.

## BeamMP doesn't start or connect

Install BeamMP separately and select its real executable. ABM starts it with `--no-launch` from the launcher's own directory. A running launcher is not a connected server. The bridge can enable an installed mod, but it can't download missing BeamMP content or bypass server restrictions.

## Reporting a problem

Use the repository's Issues tab. Include the ABM, bridge, and game versions, your platform, how you installed it, the steps you took, what you expected, what happened, and redacted diagnostics. Avoid full home paths, credentials, chat logs, usernames, server addresses, and proprietary archives. For security-sensitive reports, follow [SECURITY.md](../SECURITY.md).
