# Actual Mario PlayerUtil regression

**Final result: two fresh-process runs passed (exit 0)** for `smg-pc-mario-gateway-walk-tests --player-util`, using binary SHA-256 `a8c0121878ba6c0bed0fd34d7e94c81426e5c55717a4c7c25f69fdafd4252b45`. Both runs reached every new utility assertion, completed original scene/player retirement, and exited without a teardown crash. Exact results, durations and proof lines are in `final-player-results.json`; compile commands and source hashes are in `player-util-regression-manifest.json`.

This is a focused original-player ownership/lifecycle result. **The default full walk/release/camera fixture is not claimed as passing:** its remaining legacy packet-trace probes have no current producer and need a separate migration.

## What is verified

`tests/OriginalPlayerUtilTests.cpp` invokes the enabled original PlayerUtil TU against the real initialized MarioActor held by the scene's actual MarioHolder. The focused route runs original stage entry to controllable ground, then executes the utility helper before authored placement retirement. It schedules no movement after the original reset call.

- `MarioAccess::offControl/onControl/isOffControl` directly operate on actor `_3C0`. Disable and enable without reset are synchronous. Enable with reset executes the complete original `MarioActor::resetCondition`: movement vectors and vertical speed clear, swim damage interval and `_1C._3` clear, Xanime `_7E` is restored, and movement `_1F` is preserved.
- Position and center getters borrow actual MarioActor fields. Normal velocity borrows internal `Mario::mVelocity`, and writes through that pointer update that owner independently of LiveActor's velocity cache. Rush velocity borrows the actor's actual last displacement. Gravity borrows the vector selected by original Mario gravity logic, independently of the base LiveActor cache.
- Original `MarioAccess::setTrans` immediately updates actor/internal/camera positions, `_1C0`, and the actual translation-based dummy sensor. `_688` updates only when movement `_37` is set; both branches pass. `_2A0` remains separate, matching the source. Original HitSensorKeeper updates all registered sensors, including the invalidated dummy. Position and conditional state are restored before later checks.
- Unbound `MarioAccess::isOnGround(0)` observes the current movement `_1` bit even when the unchanged cached Binder result disagrees. Changes are visible without another tick.
- The rush branch uses an ordinary LiveActor with an actual Binder placed on a real authored KCL hit beneath Mario. `LiveActor::updateBinder` produces the contact; no synthetic contact record supplies it. The player inherits grounded state from that host while Mario's own bit is clear. Outward host velocity makes the player airborne despite retained Binder contact and a set Mario bit.
- While the actual Game allocation scope is selected, a debug event with category/name/detail longer than small-string storage is emitted. Exact content and stage identity survive. `JKRHeap::findFromRoot` confirms that the retained event vector and all four string buffers belong outside Game heaps, covering the parent's process-owned semantic-trace allocation fix.

The control flag is restored afterwards; no artificial rollback of real resetCondition status/animation transitions is attempted. Other borrowed references, velocity/cache fields and translation state are restored where appropriate.

## Actual scene/model ownership

The fixture now captures constructor- and init-created raw NameObj children under a retained real scene heap, matching Showcase. The player/holder references clear before the complete child graph retires. Scenario catalog and particle resource initialization precede scene construction. The first scene tick follows original execution-list allocation.

Original `MarioActor::initAfterPlacement` supplies `_240` and internal air/selected gravity from the authored point-gravity owner. It does not enable LiveActor's independent gravity phase; the test no longer assumes that cache/flag owns player gravity.

The authored entry selects `ステージインA` / `StageStartGround` (64 frames), with the 180-frame Wait resource retained. Readiness advances ordinary scene ticks until original input gates unlock and Mario reaches the basic animation group on ground; no animation or player state is forced. Every actual J3D joint matrix remains finite. Submitted geometry is checked through the actual NameObjListExecutor, active DrawBufferExecuter, and Mario's own J3DShapePacket/model/matrix-buffer/material-display-list identities after scene drawing. Original hidden entry frames are allowed; readiness requires nonempty visible shape submission.

The collision check retains exact provenance for all six original KCL resources. Category 0 contains four meshes/14207 triangles; original `tryCreateCollisionMoveLimit` uses category 3, which retains the other two meshes. The previous six-mesh assertion mixed those categories. Exact source paths and sizes are preserved in `player-util-collision-inventory.log` and in the fixture assertions.

## Limits and historical evidence

Each focused process constructs and tears down one complete actual scene. An attempted second RuntimeContext in the same process reached the existing singleton boundary, `Mapped resource heap cannot replace an initialized OS allocator`; the final result repeats the executable in fresh processes instead. A prior same-finalized-scene replacement attempt also correctly rejected late model registration. Neither capability was added or bypassed for this test.

The first successful helper cycle, before the singleton repeat was removed, is preserved in `player-util-first-cycle-evidence.log`. Final successful results supersede that intermediate process failure.

The default walk fixture also received mechanical migrations from removed model/BCK helpers to actual ModelManager/ResourceHolder/J3D owners, corrected original entry/blend observations, and explicit locked-swing preconditions through original MR utilities. Its remaining legacy packet probes are unresolved and its full result remains unverified. Original `基本` is a WalkSoft/Walk/Run/Wait blend; its first filename is not an authoritative currently dominant Run/Wait label.

Both changed test translation units compile with LLVM 23 optimized debug settings (exit 0, existing header warnings). No production Game source changes, commits, or Xmake configuration mutations were made for this regression cohort. The parent owned final builds and real-disc execution.
