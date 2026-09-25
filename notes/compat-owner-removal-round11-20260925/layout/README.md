# Actual layout owner restoration

Restored the complete current decompilation `LayoutManager` source/header, then enabled the original `LayoutActor` and `LayoutPaneCtrl` owners because the old pane-controller implementation depended on the alternate manager graph. The manager now builds its actual NW4R Layout through the existing canonical SDK, creates original animation transforms/players, applies original locale pruning, tracks pane/group controls, and performs the original animation/matrix/draw operations. `LayoutGroupCtrl` is also donor-complete with explicit native child destruction. No new decompilation or decomp edits were needed.

Removed four files: `compat/OriginalLayoutLocale.cpp`, all of `layout/LayoutManagerCompat.cpp`, `layout/LayoutUtilCompat.cpp`, and `layout/LayoutHost.hpp`. Seven missing MR functions returned unchanged to the already compiled canonical `Game/Util/LayoutUtil.cpp`, including the original conditional execution behavior of `setLayoutScalePosAtPaneScaleTransIfExecCalcAnim` and effect notification from `startAnim`. The native record graph no longer backs any Game LayoutManager.

## Native boundaries retained in their owners

- `LayoutManager` retains its actual `LayoutHolder` token through SDK destruction. It owns the original pane/group controllers, their links, matrix-reference arrays, layout-name bytes, animation-pointer array, and indirect TexMap. Original group registration can replace a movement slot while older group links remain reachable; every such controller stays owned until destruction.
- All original panes are recorded before locale pruning. Detached panes keep their identity for group links. Cleanup unbinds animations, retires original CustomTagProcessor objects and controller links, then frees detached roots and the SDK Layout/remaining graph while resource backing is still live. The SDK owns its actual transforms, materials, string buffers, and root tree.
- Array slots are initialized for constructor failure cleanup; manager construction unwinds owned resources. The converted basename buffer now remains alive through `initArc` instead of pointing into an ended block scope. Replacing a matrix-reference allocation retires the previous allocation.
- `LayoutActor::releaseNativeResources()` preserves the existing early process-retirement operation and is idempotent. Its destructor invokes the same real owner operation. Effect callback lifetime continues through existing EffectSystemOwnership until that independent owner is restored; the actual actor algorithm otherwise matches the donor. App/scene prepasses were migrated by the other lanes.
- `LayoutHolder::GetResource('timg', ...)` now decodes the bounded packed TPL into existing widened descriptor/backing storage and supplies `GetHostTextureResourceState` to SDK materials/animation. It preserves raw `getResOther` identity and the donor accessor's metadata word at offset 4. Archive byte size is used only for bounds. Layout/animation resources are validated against their archive span before the SDK decodes them.
- The original indirect-screen-texture selection now compares the retained decoded resource identity. Native image storage is a copy and cannot be selected using ordered packed-archive pointers or a 32-bit sentinel. The actual screen TexMap and material replacement remain the original path.
- Restored missing SDK definitions from existing donor: VEC3 constructors, complete `nw4r/math/math_types.cpp::VEC3TransformNormal`, and DrawInfo's three setters. The transform keeps the donor scalar expression order; root wires `-ffp-contract=off`.

## Remaining preview and diagnostics scope

`LayoutRuntime` and `Nw4rLayoutRecords` remain an independent preview facility used by their existing explicit callers. Removed their Game-manager parameter and locale-pruning/remapping branch, so they cannot publish an alternate graph into Game. Their general native resource decoding, fonts, textures, and rendering helpers are preserved. Removing the remaining preview pipeline is a separate scheduler/tools closure.

The obsolete global LayoutHost diagnostic API was deleted. `OriginalProcessTrace` no longer recognizes the old `SMGPC_DEBUG_LAYOUT_DUMP_*` sidecar dump variables; ordinary actor tracing remains. Root migrated scheduler diagnostics and other lanes migrated pointer/process consumers. Root also handles retired API fixture cleanup without adding new tests.

## Build and evidence

Every owned path was clean or absent before this lane. `before/`, `owned-manifest.json`, and `owned-working-delta.patch` record exact scope. `source-validation.json` compares complete donor owner methods and records the scoped whitespace check. `build-wiring.json` lists the original owner enablement and one new SDK source. The SDK member-call declaration scan found no remaining missing declaration after restoring DrawInfo setters.

No builds, test runs, Git index changes, commits, or pushes were performed by this lane. Root owns the integrated app build and short real-disc smoke. Initial compile errors were the missing native SDK vector helper and DrawInfo setters; both were repaired in their canonical SDK owner rather than substituting layout behavior.
