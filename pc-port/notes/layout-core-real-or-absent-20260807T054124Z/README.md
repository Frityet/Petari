# Layout core: exact Game boundary, real resource runtime, or explicit absence

## Outcome

The PC port no longer stores renderer/resource/animation/pane/debug state in the retail `LayoutActor`, `LayoutManager`, or `LayoutPaneCtrl` Game classes.

- `Game/Screen/LayoutActor.cpp` and `.hpp` are byte-identical to the root decompilation.
- `Game/Screen/LayoutPaneCtrl.cpp` and `.hpp` are byte-identical to the root decompilation.
- `Game/Screen/LayoutManager.hpp` is byte-identical to the root decompilation.
- There is no root `LayoutManager.cpp`; the old PC-only Game TU was removed, preserving the root source boundary.
- The exact `LayoutActor.cpp` and `LayoutPaneCtrl.cpp` are retained as source evidence but excluded from the host build because the full retail NW4R object graph (`LayoutHolder`, `PaneEffectKeeper`, `LayoutAnmPlayer`, and `StarPointerLayoutTargetKeeper`) is not present.
- Generalized host state and method implementations now live in `src/layout/LayoutHost.hpp` and `src/layout/LayoutManagerCompat.cpp`.

The host bridge is keyed by the retail objects and owns the actual parsed `LayoutRuntime`, animation mirrors, pane controls, pane matrices, and debug-only inspection data. It does not recognize title, file-select, picture-book, actor, stage, or animation names specially.

## Real-or-absent behavior

Operations are backed by the requested BRLYT/BRLAN resource. Missing resources, panes, text boxes, texture slots, active animations, configured layers, manager hosts, or tagged message templates now fail explicitly. Removed fallbacks include:

- implicit animation-layer clamping;
- synthetic zero animation frames/durations/stopped states;
- empty/no-op pane visibility, alpha, text, texture, and pane-animation mutations;
- invalid text alignment clamping;
- negative text argument success;
- blank text substituted for an absent message archive/tag;
- identity pane matrices for absent panes;
- a synthetic pane index of zero for every existing BRLYT pane;
- a host-only `LayoutManager(LayoutActor*)` class shape and PC-only Game member methods.

The renderer/resource behavior remains generalized. When retail layout archives are present, the same parsed BRLYT/BRLAN/pane/material/text pipeline drives title, file-select, and picture-book layouts. When a required retail resource is absent, the corresponding operation is absent rather than simulated.

## NW4R declaration boundary

`Game/Screen/LayoutManager.hpp` includes the retail `<nw4r/lyt/drawInfo.h>`. The port now supplies the genuine historical retail-facing declarations from `a7b59436e^` under the non-Game `src/nw4r/` compatibility include tree:

- `src/nw4r/lyt/drawInfo.h`
- `src/nw4r/math/types.h`
- `src/nw4r/ut/Rect.h`

`nw4r::lyt::DrawInfo` construction/destruction uses the real identity-view, unit-location-scale, global-alpha, and zero-flag semantics in the generalized layout implementation.

## Adjacent caller migration

The exact headers removed PC-only member APIs. Existing host-divergent callers were therefore routed through the free `smgpc::layout` bridge:

- `Game/Util/LayoutUtil.cpp`
- `Game/Util/StarPointerUtil.cpp`
- `Game/Screen/ButtonPaneController.cpp`
- `runtime/SceneScheduler.cpp`
- `runtime/RuntimeContext.cpp`

`NameObj` destruction now releases any registered layout sidecar, effect binding, scheduler entry, pane controls, and runtime through that generalized bridge. This preserves the exact inline retail `LayoutActor` destructor while preventing host state from surviving its retail owner.

Two Game Screen files gained the real `J3DAnimation.hpp` dependency they use directly after the old transitive PC-only LayoutActor fields disappeared:

- `Game/Screen/Manual2P.cpp`
- `Game/Screen/FileSelectInfo.cpp`

No already-exact Game caller was given a host workaround or PC-only class member.

## Exactness evidence

```text
b3a77ab8d38fa1f7e124bd95d43cb25bc67c8f35ae8e6119bb0a2f84cbb50d92  src/Game/Screen/LayoutActor.cpp
b3a77ab8d38fa1f7e124bd95d43cb25bc67c8f35ae8e6119bb0a2f84cbb50d92  pc-port/src/Game/Screen/LayoutActor.cpp
cf39441c8b4f6418775b8e6c516448c6f7a06ef9f954d0f46f7dc3dc6f9d85a4  include/Game/Screen/LayoutActor.hpp
cf39441c8b4f6418775b8e6c516448c6f7a06ef9f954d0f46f7dc3dc6f9d85a4  pc-port/src/Game/Screen/LayoutActor.hpp
fcb1d8d8da2a711a0c7bc96cc7bdb221d348be767b2f3a581832439db1f4e1a4  src/Game/Screen/LayoutPaneCtrl.cpp
fcb1d8d8da2a711a0c7bc96cc7bdb221d348be767b2f3a581832439db1f4e1a4  pc-port/src/Game/Screen/LayoutPaneCtrl.cpp
e5af8a2a5db8faae18a328851c8253526d0edb75dba365836b4803e318a99aa3  include/Game/Screen/LayoutPaneCtrl.hpp
e5af8a2a5db8faae18a328851c8253526d0edb75dba365836b4803e318a99aa3  pc-port/src/Game/Screen/LayoutPaneCtrl.hpp
ac2a504aac222f5eccce30c1be7fa061944b315c2124cfa4c53f734a0955d524  include/Game/Screen/LayoutManager.hpp
ac2a504aac222f5eccce30c1be7fa061944b315c2124cfa4c53f734a0955d524  pc-port/src/Game/Screen/LayoutManager.hpp
```

`git diff --check` passed for the complete layout boundary/caller/test path set.

## Verification status

The final lifecycle cleanup and all changed layout sources/adjacent consumers were verified after the shared edits settled:

```text
xmake build smg-pc-layout-real-or-absent-tests  -> build ok
xmake run smg-pc-layout-real-or-absent-tests    -> Layout real-or-absent tests passed: 6/6
xmake -vD smg-pc                                -> build ok
git diff --check                                -> clean
```
