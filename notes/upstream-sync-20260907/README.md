# Upstream synchronization - 2026-09-07

## Scope and preservation

Started from clean PC branch `960625516`, preserving the two local flattening commits. Published `decomp/pcp-decomp` first, then the preexisting PC branch. Safety branch: `codex/pre-upstream-sync-20260907`.

The root is now a native PC project, while SMGCommunity/Petari upstream is the Wii decompilation. Upstream `151d5a53a` is already contained by the `decomp/` reference. The root records the 45 incoming commits with an `ours`-strategy merge initialized without committing, then explicitly maps and merges the changed sources into their native destinations. This avoids restoring removed root Wii build/config/include trees or undoing the flattening.

All 174 changed upstream paths were inventoried in `upstream-paths.json`. The 27 with existing port counterparts receive an individual three-way merge/disposition below. New/unported Wii systems stay in the reference; copying them into the globbed native Game build would falsely imply their API dependencies are implemented.

## Source dispositions

| Native path | Resolution |
| --- | --- |
| `src/Game/AudioLib/AudBgm.hpp` | retain native concrete backend handle ABI; upstream requires unported JAISound object API |
| `src/Game/AudioLib/AudBgmRhythmStrategy.hpp` | three-way merge |
| `src/Game/AudioLib/AudBgmSetting.hpp` | three-way merge |
| `src/Game/AudioLib/AudFader.hpp` | three-way merge |
| `src/Game/AudioLib/AudWrap.hpp` | three-way merge |
| `src/Game/LiveActor/LiveActorGroup.hpp` | three-way merge |
| `src/Game/Map/KCollision.hpp` | already current |
| `src/Game/MapObj/PurpleCoinHolder.hpp` | three-way merge |
| `src/Game/NameObj/NameObjGroup.hpp` | upstream member naming and getObj; preserve native getObjectCount |
| `src/Game/Player/Mario.hpp` | three-way merge |
| `src/Game/Player/MarioActor.hpp` | three-way merge |
| `src/Game/Player/MarioConst.hpp` | already current |
| `src/Game/Player/MarioSkate.hpp` | retain fuller decompiled class; upstream exitJump already present |
| `src/JSystem/JKernel/JKRExpHeap.hpp` | retain native heap ownership ABI; helpers already implemented; no shadowing inherited allocator fields |
| `src/Game/AreaObj/AreaObjContainer.cpp` | three-way merge |
| `src/Game/AudioLib/AudBgmMgr.cpp` | three-way merge |
| `src/Game/AudioLib/AudBgmRhythmStrategy.cpp` | three-way merge |
| `src/Game/AudioLib/AudTrackController.cpp` | three-way merge |
| `src/Game/AudioLib/AudWrap.cpp` | three-way merge |
| `src/Game/LiveActor/LiveActorGroup.cpp` | three-way merge |
| `src/Game/MapObj/BrightObj.cpp` | upstream early-return cleanup; preserve native screen projection/resource functions |
| `src/Game/MapObj/Coin.cpp` | three-way merge |
| `src/Game/MapObj/CoinHolder.cpp` | three-way merge |
| `src/Game/MapObj/PurpleCoinHolder.cpp` | upstream registration helper and implicit destructor consistent with updated header |
| `src/Game/NameObj/NameObjGroup.cpp` | three-way merge |
| `src/Game/Player/MarioMove.cpp` | latest upstream retail mainMove body; preserve existing native walk provider and additionally decompiled methods |
| `src/Game/Util/JMapUtil.cpp` | retain native typed JMap provider; upstream change is source cleanup of original provider |

## Additional build repair

The existing showcase cannot compile on 64-bit hosts because MarioActor stores two Xanime object pointers in `u32` fields. `_9B8` and `_9BC` now have their concrete pointer types, and MarioTeresa assigns/reads them directly. This is a pointer-width correction, preserving the original 32-bit storage width on Wii and full pointer identity on the host; no gameplay branch is added.

The broad Aurora test used the removed `set_actor_base_matrix` API. Its matrix fixture now supplies the same matrix through the original virtual `LiveActor::getBaseMtx()` API. The Binder assertions remain unchanged.

## Checkpoint validation

- Native `smg-pc-game` and `aurora-thp` builds passed with Homebrew LLVM 23.
- Aurora standalone `gx_fifo_tests`: 255 passed; THP decoder smoke passed (agent evidence in sibling notes).
- Seven root focused tests passed: GX miscellaneous state, RGBA byte order, text encoding, scene scheduler heap lifetime, fixed clock, original JMap resources, and brightness visibility.
- GX copy-order now passes all exact dimensions/pixel assertions on Metal. The existing fixture failed to pump the window events that apply its requested viewport policy; the test now follows the normal application lifecycle. No renderer behavior or assertion was changed. See `../aurora-upstream-20260907/gx-copy-fixture.md`.
- Broad Aurora test now compiles past its obsolete matrix API but encounters the preexisting missing Game-method link failures also recorded by the flattening baseline (Mario animation/access and GameDataFunction). This is not counted as passing.
- Showcase compilation now passes the pointer-width failure and reaches the preexisting incomplete effect API (`emitEffectWithEmitterCallBack`, `setEffectHostSRT`, and emitter-returning `emitEffect`) in original MarioEffect. No new application runtime success is claimed at this merge checkpoint.
- Source-closeness audit: 737 Game files; 484 exact, 6 compile-only, 216 compatibility-temporary, 31 decomp-needed. The flattening baseline had 469 exact.
- Independent source review checked all mapped paths and verified the retail Mario mainMove body exactly matches latest upstream, while the preexisting native branch and subsequent methods remain unchanged.
- `git diff --check` passed.

The Gateway bunny chase and original Rosalina appearance remain unimplemented. This checkpoint must not be reported as completed gameplay.

## Published dependencies

- Aurora `b94a330df2b0a270ea3872ee5bc9f5147f354ba0` on `codex/macos-compat`; upstream `749d6ee` merged, 255 FIFO tests and THP decode smoke passed. `.gitmodules` now explicitly tracks this compatibility branch for future remote updates.
- Dolphin `d681903c67db45fef367bfc9bb5ad2656c09f0a2` on `master`; canonical upstream `a2efdf1197be` (429 incoming commits) merged, nested dependencies pinned, native no-GUI/test binaries built, 26 focused tests passed. Savestate format changed 190 to 192; old oracle savestates may need recapture.
- Decomp `e44d015422be86ecb1acf998baaa5cc9b82aa144` on `pcp-decomp`; latest upstream plus restored original runaway actors. Fresh Wii compiler and object comparison: collector 97.41%, rabbit 95.96%, unchanged Tico 98.82%. Details committed inside `decomp/notes/runaway-restoration-20260907/`.

The root merge commit is a validated source/dependency checkpoint. Follow-up work closes original effect APIs and programmable demo request ownership; it does not substitute host-authored Gateway triggers for the real chase.
