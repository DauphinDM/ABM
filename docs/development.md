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
- Keep dynamic text plain and ranges/collections bounded.
- Preserve legacy bridge/wire identifiers unless implementing a coordinated
  migration. Branding is ABM; see the compatibility inventory.
- Do not edit packaged Omarchy files or ship game assets.
- Do not run destructive live game actions as connectivity tests.
- Preview source identifiers intentionally have a few legacy names; don't
  blanket-replace them and disconnect running installations.

## Visual tests

See [ui-audit.md](ui-audit.md). Capture all pages/dialogs when changing shared
styles. Test narrow and wide window sizes. Use software/offscreen rendering
for reproducibility and inspect alpha separately from text legibility.
Keep private desktop captures out of the repository. Documentation screenshots
must use demo data; previews may be read from a developer's installed game.

## Private GitHub release process

1. Run the full test suite and the screenshot audit.
2. Update app/bridge/plugin/package versions together and the changelog.
3. Build the Arch package in `pkgbuild/` with `makepkg -f`.
4. Verify GitHub visibility is **PRIVATE** before pushing or publishing:
   `gh repo view DauphinDM/ABM --json visibility`.
5. Commit and push `main`, tag the tested commit (e.g. `v0.4.0`) and push the tag.
6. Publish a **prerelease inside the private repository** with a clear limitations
   section. Draft status alone is not a substitute for repository privacy.
7. Attach the tested Arch package, optional built source archive and SHA256SUMS.
8. Windows CI uploads a portable build artifact. Attach a Windows release ZIP
   only after the workflow succeeds; never claim clean-machine gameplay testing
   merely because compilation/tests passed.

GitHub automatically exposes source archives to authorized repository users.
Do not publish this project to a public package registry, GitHub Pages, or an
Omarchy package index while it is private. Public launch needs an explicit
owner decision, license/dependency review and appropriate Windows validation.

## CI

Linux CI runs the same build/test scripts inside an Arch container. Windows CI
sets up Qt/MSVC and vcpkg libarchive, tests and deploys to `dist/ABM/`. Workflows
use read-only repository permissions; release publication is an explicit owner
operation, not an automatic side effect of every push.
