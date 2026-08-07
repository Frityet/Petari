# Layout button/effect fallback removal

## Outcome

The retail-facing button controller boundary is restored and three layout fallbacks are removed:

- `pc-port/src/Game/Screen/ButtonPaneController.cpp` is byte-identical to `src/Game/Screen/ButtonPaneController.cpp`.
- `pc-port/src/Game/Screen/ButtonPaneController.hpp` is byte-identical to `include/Game/Screen/ButtonPaneController.hpp`.
- Button lifecycle registration and debug nerve naming no longer alter the retail Game source or class API. Optional host inspection remains a generalized `src/layout` concern and no longer injects lifecycle hooks into Game.
- Asking whether a null `LayoutActor` is dead now fails explicitly; null is no longer converted to the ordinary retail `dead` state.
- `LayoutActor::initEffectKeeper` and `LayoutRuntime::initEffectKeeper` require a non-empty named owner and an active `RuntimeContext`. Missing effect infrastructure no longer produces a successful no-op.

The title, file-select, and picture-book Game actors continue to use the same generalized layout/effect bridge when real resources and a runtime exist. This change does not introduce sequence-specific behavior.

## Aurora ownership boundary

The compatibility split is intentional:

- Keep `LayoutHost.hpp`, `LayoutManagerCompat.cpp`, and `LayoutRuntime.{hpp,cpp}` in pc-port. They bind SMG retail classes, `RuntimeContext`, scene/effect ownership, SMG layout archive discovery, and Game-facing animation/pane orchestration.
- Keep `LayoutResourceResolver` in pc-port for now. Its exact BRLYT/BRLAN path policy is reusable, but its API is currently coupled to `smgpc::resource::RarcEntry` and the surrounding SMG archive workflow.
- `BrlanAnimation.{hpp,cpp}` is the cleanest next Aurora promotion candidate. It is a standalone binary-format parser/evaluator over standard C++ spans and data types and contains no SMG sequence or runtime policy. Promote it as an isolated Aurora API plus parser tests rather than hiding that ownership change inside this fallback fix.
- `BrfntFont`, `BrlytLayout`, and `LytTexMap` are also genuinely generic NW4R-format candidates, but not yet clean slices: they currently depend on pc-port TPL decoding, GX material state, BMG text types, or `smgpc` resource objects. Those dependencies should be separated before promotion.
- The historical `src/nw4r/{lyt,math,ut}` declarations are generic platform compatibility candidates once Aurora has a stable NW4R/revolution include boundary.

No Aurora code was changed in this slice. Moving the SMG host/runtime bridge itself would put game-specific orchestration into the platform layer, while promoting the coupled parsers now would create a cross-project API migration unrelated to the exactness fix.

## Exactness evidence

```text
d25c13010c59ca5d23aaf02660bc72a00149be6075e14f8871ce5d62faa5cd0a  src/Game/Screen/ButtonPaneController.cpp
d25c13010c59ca5d23aaf02660bc72a00149be6075e14f8871ce5d62faa5cd0a  pc-port/src/Game/Screen/ButtonPaneController.cpp
9af6c4cf1b40a419e40dde309c6d886d665520e88785f86729a28437eb4374a2  include/Game/Screen/ButtonPaneController.hpp
9af6c4cf1b40a419e40dde309c6d886d665520e88785f86729a28437eb4374a2  pc-port/src/Game/Screen/ButtonPaneController.hpp
```

## Focused regression coverage

`tests/LayoutRealOrAbsentTests.cpp` now verifies that:

- a null actor is explicitly unavailable rather than reported dead;
- `LayoutRuntime` effect initialization fails without an active runtime;
- an unnamed layout effect owner is explicitly unavailable;
- `LayoutActor` effect initialization fails without an active runtime after its layout manager has been initialized.

## Verification

The focused layout translation units compiled successfully during the first build attempt. That attempt then stopped in concurrent, unrelated `SaveDataService` edits before link. After the shared tree settled, focused and aggregate verification passed:

```text
xmake build smg-pc-layout-real-or-absent-tests  -> build ok
xmake run smg-pc-layout-real-or-absent-tests    -> Layout real-or-absent tests passed: 7/7
xmake build smg-pc                              -> build ok
cmp root/PC ButtonPaneController.cpp            -> identical
cmp root/PC ButtonPaneController.hpp            -> identical
git diff --check                                -> clean
```
