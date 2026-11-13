# Developing and releasing ABM

## Build and test loop

```sh
./bin/build
./bin/test
./build/abm open --demo
```

`abm.pro` compiles C++17, Qt Quick/Controls/Network, and libarchive. `bin/test` builds the Qt tests offscreen, then runs the LuaJIT bridge tests, Node plugin tests, Python CLI tests, and standalone lifecycle tests. Tests isolate settings and use temporary sockets or ephemeral UDP ports. They never touch the live game receiver.

Headless tests set `BEAMSIDE_CONFIG_DIR`, `BEAMSIDE_RUNTIME_DIR`, and `BEAMSIDE_SKILL_DIR` directly. The last variable selects the absolute directory containing the installed `SKILL.md`. Without it, the normal destination is `~/.agents/skills/abm/`. Setting `HOME` alone doesn't redirect Qt's native Windows profile lookup. Never let CLI tests install into your real skill folder.

During incremental development, generated QML resources sometimes need a timestamp update: run `touch src/resources.qrc src/main.cpp` before building. A clean CI checkout doesn't share stale objects.

## Project conventions

- C++ owns state, files, validation, and external processes. QML owns layout.
- Use **Basic** styling, not Material. Keep compact square corners and opaque text.
- The `MotorsportGlass` style name stays for compatibility. Its current look is solid dark surfaces with restrained borders, not decorative glass. Use `SectionPanel` for padded, content-sized groups. Keep safety explanations readable instead of squeezing every control above the fold.
- Keep dynamic text plain, and give ranges and collections a size limit.
- Keep legacy bridge and wire identifiers unless you're doing a coordinated migration. Branding is ABM. See the compatibility inventory.
- Don't edit packaged Omarchy files or ship game assets.
- Don't run destructive live game actions as connectivity tests.
- Preview source identifiers have a few legacy names on purpose. Don't blanket-replace them and disconnect running installations.

## Visual tests

The offscreen platform still needs installed fonts. Linux CI installs `noto-fonts`. A fontless container can render fallback glyphs with invalid layout metrics and fail the narrow-window checks. Keep the geometry assertions instead of treating that container as a representative desktop.

See [ui-audit.md](ui-audit.md). Capture all pages and dialogs when you change shared styles. Test narrow and wide window sizes. Use software or offscreen rendering for reproducibility, and check alpha separately from text legibility. Keep private desktop captures out of the repository. Documentation screenshots must use demo data. Previews may come from a developer's installed game.

## GitHub release process

1. Run the full test suite and the screenshot audit.
2. Update the app, plugin, and package versions plus the changelog. Bridge versions describe protocol compatibility. Keep 0.11.0 until that contract changes.
3. Build the Arch package in `pkgbuild/` with `makepkg -f`.
4. Check the intended GitHub visibility before pushing or publishing: `gh repo view DauphinDM/ABM --json visibility`.
5. Commit and push `main`, tag the tested commit (for example `v0.13.0`), and push the tag.
6. After all workflows pass, publish a **prerelease** with a clear limitations section. Public visibility has been authorized for preview releases.
7. Attach the tested Arch package, the optional built source archive, and SHA256SUMS.
8. Windows CI uploads a portable build artifact. Attach a Windows release ZIP only after the workflow succeeds. Never claim clean-machine gameplay testing just because compilation and tests passed.

Public visibility exposes repository history and existing releases as well as the latest source. Check for credentials and personal data before changing it. Package registries, GitHub Pages, and Omarchy package-index publication are separate decisions, not side effects of a GitHub release.

## CI

Linux CI runs the same build and test scripts inside an Arch container. Windows CI sets up Qt/MSVC and vcpkg libarchive, runs tests, and deploys to `dist/ABM/`. Workflows use read-only repository permissions. Publishing a release is an explicit owner action, not something every push does automatically.
