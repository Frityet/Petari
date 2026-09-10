# Initialized idle layout animation controllers — 2026-09-10

## Original evidence

- `decomp/src/Game/Screen/LayoutPaneCtrl.cpp`: constructor creates one `LayoutAnmPlayer` for every configured layer; `getFrameCtrl` returns its embedded controller without requiring a current transform.
- `decomp/src/Game/Animation/LayoutAnmPlayer.cpp`: construction sets the transform/name null and initializes `mFrameCtrl(0)`. Movement and reflection skip a null transform. `isStop` returns true when the transform is null, regardless of rate. `stop` directly stores rate zero. `start` initializes the controller using the real transform duration and resets frame/rate to 0/1, choosing loop mode from that transform.
- `decomp/src/JSystem/J3DGraphAnimator/J3DAnimation.cpp`: `J3DFrameCtrl::init(0)` gives attribute LOOP, state/start/end/loop/frame zero, rate one.
- `decomp/src/Game/Util/LayoutUtil.cpp`: frame/rate setters and frame/end getters directly access `getAnimCtrl` or `getPaneAnimCtrl`; they do not require a bound BRLAN.
- `StarPointerLayout::tearDownHide` sets rate one before its layer has started an animation. The rejected valid call was captured in `notes/demo-camera-jump-20260910/after-native-layout-stack.log`.

## Change

Valid initialized root and named pane layers expose idle frame/rate controls. They remain stopped and do not advance or bind an animation when their animation name is empty. Starting a real animation retains resource lookup and resets original controller fields. Root control access through MR and LayoutPaneCtrl now returns the same controller, and direct controller frame/rate edits synchronize with native setters/getters while idle as well as active.

Root controller operations still require a real BRLYT root, configured layers remain bounds checked, and named pane operations require a real parsed pane. Missing resources do not acquire placeholder controllers or durations. Existing host allocation guards are preserved; Game sources are unchanged.

The native root loop default matches `J3DFrameCtrl(0)`. Original start overwrites that default with the actual BRLAN's loop policy. No unrelated animation advancement or interpolation behavior is changed.

## Validation

`LayoutRuntime.cpp.json/log` and `LayoutManagerCompat.cpp.json/log`: both production translation units independently compile, exit 0. `fixture-compile.json/log`: changed original wipe ownership fixture compiles, exit 0.

`tests/OriginalSceneWipeOwnerTests.cpp` exercises real disc-backed GameOver and WipeRing root/named pane controls before their first animation: shared root controller identity, original defaults, frame/rate mutation, stopped/no-binding behavior, direct controller writes, idle stop, and default restoration when the original GameOver nerve later starts its actual animation. The fixture retains absent-owner and invalid-layer rejection checks and the existing full transition/teardown checks.

Runtime target: `smg-pc-original-scene-wipe-owner-tests` with `SMGPC_REAL_DISC` naming the real RMGK01 RVZ. Final validation completed after the demo was packaged: the full target builds and links with exit 0, and the real-RVZ runtime exits 0. `runtime-build.log`, `runtime.log`, and `runtime-proof.json` preserve the delegated run commands and binary hash. All idle-controller assertions, original animation transitions, and repeated ownership checks passed; no resource skip path was used.
