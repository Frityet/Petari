# Runtime / resource / Aurora compatibility boundary review — 2026-09-19

Read-only production audit. No production source, game state, build output or Git index was changed; no game/GPU process or native build was launched. This is a bounded source review, not a full equivalence proof. Severity below expresses compatibility/correctness priority, not a demonstrated cause of the last Gateway route result.

## Findings

### P2 — live actor gravity is calculated through a second numerical algorithm

**Locations:** `src/compat/ActorMotionCompat.cpp:10–16,23–31`; called by `src/Game/LiveActor/LiveActor.cpp:66` and `src/compat/GameActorPhysicsCompat.cpp:169–174`.

The native helper normalizes the actual gravity manager's result again using `std::sqrt`, a squared-length cutoff of `1e-12`, and a finite-value gate. It also skips the query when the actor is dead. Original `LiveActor::movement` (`decomp/src/Game/LiveActor/LiveActor.cpp:119`) calls `MR::calcGravity` when the original calculation flag is set; original `MR::calcGravity` (`decomp/src/Game/Util/LiveActorUtil.cpp:2170`) stores the returned non-near-zero vector directly. That original helper is already implemented in `src/compat/GameGravityCompat.cpp:234–241`. The duplicate path can change float bits and exceptional/near-zero behavior for all affected actors, rather than supplying an architecture/platform facility.

Actual callers include the original RunawayRabbit initializers (`src/Game/NPC/RunawayRabbit.cpp:79,104`), DemoRabbit (`src/Game/NPC/DemoRabbit.cpp:73`), and PunchingKinoko's ground checker (`src/Game/MapObj/PunchingKinoko.cpp:35`), as well as other actors. This establishes reachability of the native helper, **not evidence that this difference caused a rabbit or terrain failure**.

**General resolution:** preserve the original `MR::calcGravity` call and storage semantics; keep heap/scene ownership hooks separate. A focused comparison should cover no-field, near-zero, ordinary normalized manager output, original calculation-flag state and float-bit preservation before changing the live path.

**False positive explicitly rejected:** original `onCalcGravity` is *not* flag-only. `decomp/src/Game/Util/LiveActorUtil.cpp:2687–2693` already calls `calcGravity` immediately for a live actor and then sets the flag. Immediate calculation itself is faithful. Native setter order differs, but no independent observable bug from that ordering was established here.

### P2 — one LiveActor easing wrapper bypasses original JMath table semantics

**Location:** `src/compat/LiveActorUtilCompat.cpp:341–351`.

`MR::calcNerveEaseInRate(const LiveActor*,s32)` duplicates the clamp/rate calculation and computes `1 - std::cos(rate * halfPi)`. The canonical donor (`decomp/src/Game/Util/LiveActorUtil.cpp:1837–1838`) calls `getEaseInValue(calcNerveRate(...),0,1,1)`. Existing native `src/compat/GameMathCompat.cpp:390–392` implements that shared helper with `JMACosRadian`; `src/JSystem/JMath/JMATrigonometric.hpp:64–67,168–169` uses the original quantized sine/cosine table. libm and the discrete table are not equivalent for non-table-aligned angles. This silently substitutes animation interpolation behavior even though the correct shared path exists.

An explicit source caller is `src/Game/Map/SphereSelectorHandle.cpp:340`; this audit does not claim an exercised Gateway call to this overload. The LayoutActor and NerveExecutor overloads retain the shared helper path.

**General resolution:** use the original one-line donor wrapper; validate several non-table-aligned rates against `MR::getEaseInValue` and endpoint behavior. No actor-specific condition is needed.

### P2 — EFB texture copy contains an inherited global alpha overwrite workaround

**Location:** `aurora/lib/dolphin/gx/GXFrameBuffer.cpp:311–321`.

When destination alpha and alpha update are enabled, `copy_tex` queues a framebuffer alpha clear *before* resolving the copy, including `clear == false`. The empty `if (!clear)` TODO acknowledges uncertainty, but execution still overwrites EFB alpha. A no-clear copy after changing destination alpha can therefore alter previously drawn pixels, and the copied alpha is rewritten to the current destination-alpha value rather than preserving the stored EFB content.

The original SDK `decomp/src/RVL_SDK/gx/GXFrameBuf.c:339–402` issues the copy command and uses the explicit `clear` bit; it does not draw an alpha overwrite first. The local Dolphin software reference applies destination alpha at fragment write time (`dolphin/Source/Core/VideoBackends/Software/SWEfbInterface.cpp:435–448`), with copy clearing handled separately by `EfbCopy.cpp`. Aurora already has per-draw destination-alpha plumbing at `aurora/lib/gx/gx.cpp:165–171` and `aurora/lib/gx/pipeline.cpp:44–46`; correctness of that plumbing should be checked instead of depending on a later copy to repair alpha.

**General resolution:** validate destination alpha in the ordinary pixel pipeline and remove the copy-time global repair. A meaningful regression draws two regions with different stored alpha, changes destination-alpha state, performs `GXCopyTex(...,GX_FALSE)`, and compares both the copy and a subsequent EFB read/copy. Verify true-clear uses the original copy-clear alpha independently. No runtime screenshot defect is attributed to this finding without that test.

### P3 — retained alternate EffectService fabricates a particle for absent dynamics

**Location:** `src/runtime/RuntimeServices.cpp:2303–2319` (related resolution behavior at `2262–2274`).

The alternate host particle simulation emits a one-frame particle with made-up position/alpha behavior when its resource pointer or dynamics block is absent. Its `resolve` path also returns an empty result without a resource library, while `emit` still records an active effect/event (`1897–1940`). These are explicit silent substitute behaviors, rather than rejecting an unsupported resource or using the actual JPA owner.

**Reachability limit:** this is compiled production service code, and `SceneScheduler.cpp:1142–1144` retains its draw hook, but the repository search found no current `src/` caller of `RuntimeContext::emit_effect` beyond its definition. Current `LayoutActor::initEffectKeeper` uses original PaneEffectKeeper (`src/Game/Screen/LayoutActor.cpp:117–119`), and current LiveActor routes through the actual effect owner (`src/Game/LiveActor/LiveActor.cpp:342–343`). The host service is directly exercised by older focused tests. Therefore this is retained duplicate-system/cleanup debt, **not proof that the current original-process replay fabricates these particles**.

**General resolution:** remove the obsolete alternate simulation/service references once caller inventory confirms none are needed, or route retained generic tooling through the real JPA resource/owner API. Do not retain fabricated successful feedback as a fallback.

## Checks that did not produce workaround findings

- Searches for `HeavensDoor`, `Gateway`, `RunawayRabbit`, `Rosetta`, `CrystalCage` and `WarpPod` under `aurora/lib`, `src/compat`, `src/resource` and `src/runtime` found no stage-specific gameplay branch. The WarpPod hits are the original SceneObj factory type and owner name. This lexical scan cannot establish equivalence of every general algorithm.
- `src/compat/ModelCreationCompat.cpp:30–39` contains an explicit Mario/Luigi/model-name list, but the same list and selection are present in `decomp/src/Game/Util/ModelUtil.cpp:594–603`. It is original policy, not an added Mario rendering workaround.
- `OceanHomeMapFunction::tryEntryOceanHomeMap` in `src/compat/PlanetMapRuntimeCompat.cpp` matches the donor's two named planet cases, then reports the missing original controller with an exception. This is a documented fail-closed unsupported boundary, not a silent successful replacement. Full OceanHome controller support remains a separate closure task.
- Gravity queries use the actual scene-owned PlanetGravityManager (`src/compat/GameGravityCompat.cpp:18–48`); no hardcoded planet center/radius or Gateway gravity is supplied. Parameter defaults and type strings mirror the donor GravityUtil policy. The separate numerical wrapper finding above remains.
- BinderCompat owns deletion of the original Binder plane allocation and an actual offset binding; the complete original Binder implementation supplies collision behavior. There is no actor-specific Binder bypass in this file.
- Native J3D/RARC resource construction and ownership use real mounted data and throw for absent required owners. Some null returns are original format-probe outcomes, not synthesized models. No fake model success was established by this review.
- Controller waypoints, rabbit route order, extra A presses and world-coordinate matching exist in `notes/.../operate_first_catch.py` and its JSON plan, not production providers. These are explicitly external controller diagnostics. They still need to be described as scripted-controller evidence, not physical keyboard play or original gameplay implementation.

## Scope and next work

Reviewed broad literal/fallback searches plus targeted actor motion, gravity, live-actor utilities, model creation, resource owners, shadow CSV construction, original effect ownership versus the alternate host service, and Aurora GX-copy code against local SDK/Dolphin references. No complete decoder/renderer/system audit or new numerical/pixel execution proof was attempted. Prioritize the active generic divergences above with original-source comparisons and focused tests; retain no stage-specific fixes in resolving them.
