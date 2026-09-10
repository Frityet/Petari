# Read-only review: local NW4R pane animation publication

Reviewed the frozen `Nw4rLayoutRecords`/compat performance diff and added `OriginalLayoutGroupTests` phase assertions against original `decomp/src/nw4r/lyt/lyt_pane.cpp`. No production/test edits or builds were performed for this review. Exact source bytes are recorded in `source-manifest.json`; parent owns linked runtime and performance measurement.

No introduced correctness or ownership blocker found.

- Original `Pane::AnimateSelf` (lines258–270) applies enabled local animation links, then eligible material animation; it does not update local/global matrices. New `animate_pane` imports and publishes only its selected record, passes `false` for matrices, and updates only that record's previous-local-state snapshot. This preserves pending direct writes on other panes and stops a root/sibling AnimateSelf call from publishing unrelated properties.
- Full synchronization (native lines124–132) still imports every pane before publishing any pane. Therefore direct SDK edits to a parent are present in runtime state before a child's recursive full3D matrix/alpha calculation; deferred child edits are imported even if a different sibling just animated. Follow transforms remain in the same runtime map; later `LayoutManager::calcAnim` clears/rebuilds follow positions in original control order, then refreshes matrices. The optimization does not cache global matrices across those changes.
- Original `CalculateMtx` reads the local values and builds T*Rz*Ry*Rx*S, then parent composition (lines176–200). New AnimateSelf leaves both matrices untouched until the retained matrix publication phase. The added synthetic assertions check this phase distinction, sibling isolation and a deferred direct Child translation. Existing real FlyMeter draw coverage remains a separate integration test.
- The cached pane index refers to immutable vector order; hierarchy/name edits are still rejected, pane objects remain separately owned by unique_ptr, and dynamic_cast plus owner identity rejects records belonging to another layout. No index or storage lifetime changes beyond that lookup optimization.

## Existing limitations, not introduced by this diff

1. Native `Pane::Animate` recursively animates children unconditionally (compat lines57–59), whereas original lines251–254 skip child recursion when the pane is invisible and option bit1 is set. The actual manager uses its own `animateRecursive` path. This review did not broaden scope to alter recursion.
2. Full synchronization still re-evaluates active BRLAN through `animationFrameForPane`, which overlays active animation values over committed SDK writes (LayoutRuntime lines3631–3652). Thus a direct write to an actively animated transform channel after AnimateSelf can be overwritten at matrix publication; original CalculateMtx would read the current SDK property without replaying animation. This behavior already existed in the previous whole-layout synchronization. The new synthetic phase test has no active BRLAN on its modified channels and does not prove that separate case.

Both existing limitations were communicated to parent and the implementation agent; they are not being presented as regressions introduced by this performance change.

## Subsequent parent runtime validation

The frozen pane cohort later built and ran in `smg-pc-original-layout-group-tests` under LLVM23 O2, both exit0, including the new phase assertions and actual real-disc FlyMeter draws. Binary SHA `9ca390243b48a25dae8248629f253aea6f1dfc4de421c1d95f212d7bf051944c`; exact result `../preview-fps-crash-20260910/optimized-tests.json`. The combined production run also completed1200 ticks and exited0 (`optimized-debug.json` in that folder). This runtime evidence does not eliminate the explicitly listed pre-existing semantic limitations.
