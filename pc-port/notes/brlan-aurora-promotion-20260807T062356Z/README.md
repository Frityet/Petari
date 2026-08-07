# BRLAN runtime promotion to Aurora

Timestamp: `2026-08-07T06:23:56Z`

## Outcome

The standalone BRLAN parser/evaluator moved from the PC-port layout implementation into Aurora as `aurora::nw4r::lyt`. The PC port now consumes that API directly. There is no compatibility alias, shadow implementation, synthetic fallback, or title-specific adapter.

Aurora commit `af89a3289ad73bddeb156d2718bbd05d6ddf6afd` (`Add generalized NW4R BRLAN runtime`) was pushed as a fast-forward to `origin/main`. The parent repository's `pc-port/aurora` pointer was deliberately left unstaged and uncommitted.

## Ownership evidence

Before promotion, `src/layout/BrlanAnimation.cpp` included only its own header and standard C++ headers. Searches of the implementation and API found no dependency on SMG game code, `Game/`, title, file-select, picturebook, runtime context, archives, rendering, or resource routing. A standalone C++20 syntax compile passed.

The Aurora implementation is now isolated in:

- `include/aurora/nw4r/brlan.hpp`
- `lib/nw4r/brlan.cpp`
- standalone CMake target `aurora::nw4r`
- standalone xmake target `aurora-nw4r`

A post-move search of the Aurora header, source, and focused tests for `smgpc`, `Title`, `FileSelect`, `PictureBook`, `RuntimeContext`, `LayoutRuntime`, `Game/`, `resource/`, and `render/` returned no matches.

The PC-port duplicate `src/layout/BrlanAnimation.{cpp,hpp}` was deleted. `LayoutRuntime` and the two probes use `aurora::nw4r::lyt` directly. A source search found no remaining live reference to `layout/BrlanAnimation`, `smgpc::layout::Brlan*`, or `smgpc::layout::parse_brlan_animation`.

No `src/Game` C++ source was changed for this promotion. The only Game-tree wiring is the `aurora-nw4r` target dependency in `src/Game/xmake.lua`, because the current build target compiles the external `src/layout` runtime alongside the Game library.

## Verification

### Aurora

- `xmake build aurora-nw4r`: passed.
- CMake 3.31.6 configure with GX/DVD/CARD/RmlUi disabled: passed. The host CMake 3.22 was below Aurora's declared 3.25 minimum, so the official CMake binary was used from a temporary directory without modifying the repository.
- CMake target `brlan_tests`: built.
- `ctest --test-dir /tmp/aurora-brlan-cmake.x1IWaa --output-on-failure -R Brlan`: 4/4 passed.
- Focused coverage: minimal big-endian RLAN/pai1 parsing; missing magic, wrong endian, and truncated-block rejection; pane Hermite/step/alpha/visibility sampling; texture and material sampling.
- `git diff --cached --check`: passed before commit.
- `git fetch origin main` showed the expected base `afdf18076d3588a3a844bd5a1c77b02ddb22ce1a`; push was a non-force fast-forward to `af89a3289ad73bddeb156d2718bbd05d6ddf6afd`.

### PC port

- `xmake build smg-pc-layout-real-or-absent-tests`: passed.
- `xmake run smg-pc-layout-real-or-absent-tests`: `Layout real-or-absent tests passed: 7/7`.
- `xmake build smg-pc-layout-probe`: passed.
- `xmake build smg-pc-picturebook-resource-probe`: passed.
- `xmake build smg-pc`: passed and linked the aggregate executable.
- Direct execution of the built layout probe from the PC-port root passed against Korean disc resources:
  - `TitleLogo`: 24 panes, 13 pictures, 13 materials, 5 BRLAN animations.
  - `FileSelect`: 55 panes, 21 pictures, 19 text boxes, 40 materials, 6 BRLAN animations.
  - `PictureBook`: 236 panes, 92 pictures, 82 text boxes, 174 materials, 10 BRLAN animations.
- `xmake run smg-pc-picturebook-resource-probe`: passed against `/workspaces/pcport/orig/RMGK02/files`:
  - parsed all 10 PictureBook BRLANs;
  - parsed the 1869-frame PrologueDemo animation;
  - parsed all 7 PrologueStarSteward animations;
  - decoded every checked picturebook/chapter BTI archive with visible pixels;
  - parsed the RosettaPictureBook J3D model;
  - verified both prologue THP movies and the Korean message archive.

The layout probe's `xmake run` wrapper starts the executable in the build directory, while that older probe has a private root search that does not walk far enough upward. Its already-built binary was therefore run from `/workspaces/pcport/pc-port`; all three real-resource probes passed. The shared `DebugPaths`-based picturebook probe works through `xmake run` normally. This pre-existing probe-launch issue was not hidden with another compatibility fallback or folded into the BRLAN promotion.

Generated detailed probe reports are in `.cache/layout-probes/TitleLogo.md`, `.cache/layout-probes/FileSelect.md`, and `.cache/layout-probes/PictureBook.md`.
