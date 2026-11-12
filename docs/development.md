# Developing and releasing ABM

## Build/test loop

```sh
./bin/build
./bin/test
./build/abm open --demo
```

`abm.pro` compiles C++17, Qt Quick/Controls/Network and libarchive. `bin/test`
builds Qt tests offscreen, then runs LuaJIT bridge tests, Node plugin tests,
Python CLI tests and standalone lifecycle tests. Tests isolate settings and
use temporary sockets/ephemeral UDP ports, never the live game receiver.

Generated QML resources sometimes need an explicit timestamp update during
incremental development: `touch src/resources.qrc src/main.cpp` before build.
A clean CI checkout does not share stale objects.

## Project conventions

- C++ owns state, files, validation and external processes; QML owns layout.
- Keep **Basic**, not Material styling; compact square corners and opaque text.
- The `MotorsportGlass` style name is retained for compatibility. Its current
  appearance is solid dark surfaces with restrained borders, not decorative
  glass. Use `SectionPanel` for padded, content-sized groups and keep safety
  explanations legible rather than squeezing every control above the fold.
- Keep dynamic text plain and ranges/collections bounded.
- Preserve legacy bridge/wire identifiers unless implementing a coordinated
  migration. Branding is ABM; see the compatibility inventory.
- Do not edit packaged Omarchy files or ship game assets.
- Do not run destructive live game actions as connectivity tests.
- Preview source identifiers intentionally have a few legacy names; don't
  blanket-replace them and disconnect running installations.

## Visual tests

The offscreen platform still needs installed fonts. Linux CI installs `noto-fonts`;
a fontless container can render fallback glyphs with invalid layout metrics and
fail the narrow-window checks. Keep the geometry assertions, rather than treating
such a container as a representative desktop.

See [ui-audit.md](ui-audit.md). Capture all pages/dialogs when changing shared
styles. Test narrow and wide window sizes. Use software/offscreen rendering
for reproducibility and inspect alpha separately from text legibility.
Keep private desktop captures out of the repository. Documentation screenshots
must use demo data; previews may be read from a developer's installed game.

## GitHub release process

1. Run the full test suite and the screenshot audit.
2. Update app/plugin/package versions and the changelog. Bridge versions describe
   protocol compatibility: keep 0.11.0 until that compatibility contract changes.
3. Build the Arch package in `pkgbuild/` with `makepkg -f`.
4. Verify the intended GitHub visibility before pushing or publishing:
   `gh repo view DauphinDM/ABM --json visibility`.
5. Commit and push `main`, tag the tested commit (e.g. `v0.12.0`) and push the tag.
6. Publish a **prerelease** with a clear limitations section after all workflows
   pass. Public visibility was explicitly authorized for the 0.12.0 preview.
7. Attach the tested Arch package, optional built source archive and SHA256SUMS.
8. Windows CI uploads a portable build artifact. Attach a Windows release ZIP
   only after the workflow succeeds; never claim clean-machine gameplay testing
   merely because compilation/tests passed.

Public visibility exposes repository history and existing releases as well as
the latest source. Review for credentials and personal data before changing it.
Package registries, GitHub Pages and Omarchy package-index publication remain
separate decisions, not side effects of a GitHub release.

## CI

Linux CI runs the same build/test scripts inside an Arch container. Windows CI
sets up Qt/MSVC and vcpkg libarchive, tests and deploys to `dist/ABM/`. Workflows
use read-only repository permissions; release publication is an explicit owner
operation, not an automatic side effect of every push.
