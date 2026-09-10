# Camera/player compatibility cleanup audit — 2026-09-10

Read-only review. No production edits, build, runtime, or commit was performed for this audit. Root HEAD at inspection: `270be3aab62361d8a4f9bec49b39ed2bd43060b7`; decomp HEAD: `db38bcbac05d051fa9b319306a7ba43cb84f37ae`. Existing movement/camera replay evidence belongs to its earlier checkpoint and is not a test of the changes proposed here.

## 1. Replace player shadow-state wrappers with the actual Mario owner

This is the highest-impact semantic cleanup. The full original `src/Game/Player/MarioAccess.cpp` and `MarioActorGravity.cpp` are now compiled in the normal Game archive. `src/Game/xmake.lua:18–21` excludes only MarioSound and MarioState from the Player directory. However, original `Util/PlayerUtil.cpp` remains excluded, and its compatibility providers retain an earlier generic LiveActor/PlayerSystemService model.

| Actual provider | Current behavior | Available original behavior |
| --- | --- | --- |
| `src/compat/PlayerStateCompat.cpp:37–46`, then `src/runtime/RuntimeServices.cpp:3638–3645` | `offPlayerControl`/`onPlayerControl` only set `_control_enabled` and queue `_reset_condition_requested`; `isOffPlayerControl` reads that host boolean. | `decomp/src/Game/Util/PlayerUtil.cpp:773–782` calls `MarioAccess::{offControl,onControl,isOffControl}`. Actual `src/Game/Player/MarioAccess.cpp:476–490` changes MarioActor `_3C0`; `onControl(true)` calls `resetCondition()` immediately. |
| `src/compat/PlayerUtilCompat.cpp:107–122` | `setPlayerPos(vec)` copies a matrix, replaces its translation, and passes it to the host matrix setter. | Original PlayerUtil line75 calls `MarioAccess::setTrans`. Actual MarioAccess line262 updates actor position, internal Mario position, conditional `_688`, `_1C0`, camera translation, and all hit sensors. |
| `src/compat/PlayerUtilCompat.cpp:229–236`, then `RuntimeServices.cpp:3602–3625` | `setPlayerBaseMtx` uses generic `MR::setBaseTRMtx`, writes actor translation, and sets host forced-matrix flags. | Original PlayerUtil line798 calls actual `MarioAccess::setBaseMtx`, which calls `MarioActor::forceSetBaseMtx` (`MarioAccess.cpp:559`). |
| `src/compat/PlayerStateCompat.cpp:20–22`, then `RuntimeServices.cpp:3691–3693,3802–3816` | `isOnGroundPlayer` reads a snapshot of native Binder contacts. | Original PlayerUtil line23 calls `MarioAccess::isOnGround(0)`: the actual Mario movement grounded flag, or the rush owner's ground result (`MarioAccess.cpp:50–59`). |
| `src/compat/PlayerUtilCompat.cpp:134–136` | `getPlayerGravity` returns the base LiveActor `mGravity` address. | Original PlayerUtil line111 returns `&MarioActor::getGravityVec()`, which returns Mario's selected gravity vector (`MarioActorGravity.cpp:11–12`). |

These are source-confirmed differences, not a claim that each currently breaks the playable demo. Searching production callers found no application of the host control/reset flags back to Mario: `consume_reset_condition_request` has no caller outside its declaration/definition. The service's control boolean is read by the native demo adapter, but that does not write `_3C0`. Ground-state wrappers are used by real SubMeterLayout, CounterLayoutController, SwitchArea and RestartCube. This makes the difference relevant beyond the bounded movement loop.

Retail corroboration: `notes/gateway-audit-20260907/restoration/retail/asm/Game/Util/PlayerUtil.s` has direct branches to MarioAccess for `isOnGroundPlayer` at `803F3500`, `setPlayerPos` at `803F361C`, and off/on/isOff control at `803F49B4/49B8/49BC`. `getPlayerGravity` calls MarioActor::getGravityVec at `803F36F8`.

**Minimal integration:** make the original MarioHolder/MarioActor the gameplay authority for this cohesive query/control/placement group; remove the host shadow writes and cached answers from the MR providers. Prefer enabling the existing original PlayerUtil TU and removing overlapping definitions together. If its full closure requires a larger follow-up, the immediate seam can be the already available original MarioAccess calls; do not introduce another gameplay implementation. Keep host service snapshots for diagnostics/presentation only. Inventory all overlapping MR definitions before enabling the TU, because several compatibility files currently split its symbols.

**Validation needed:** use an actual scene-owned MarioHolder/MarioActor. Assert control flag mutation/reset timing, actor/internal/camera position agreement after placement, gravity pointer identity and current grounded state without an intervening service snapshot; include the rush branch when a real owner fixture is available. Re-run the existing 960-tick idle/WASD/release/two-jump-and-land replay and clean shutdown. `PlayerUtilRealOrAbsentTests.cpp` tests missing ownership; `PlayerActorBridgeTests.cpp` deliberately uses generic LiveActors and currently tests a service interface, so it cannot prove the original Mario semantics. Move those generic checks to the service API if the MR interface adopts its real owner. Preserve a clear missing-owner boundary at the scene owner lookup rather than fabricating Mario state.

**Match status:** decomp configure.py marks PlayerUtil, MarioAccess and MarioActorGravity TUs `NonMatching`; this audit does not claim a new percentage or that every body in these TUs has retail parity. The small wrapper calls above were checked against retail assembly, and their original target bodies are present. The broader TU needs its normal reference and native checks before activation.

## 2. Replace host screen-projection arithmetic with original CameraUtil

`src/compat/CameraUtilCompat.cpp:630–709` and private `project_world_to_screen`/`unproject_screen_to_world` still calculate from RuntimeContext's cached `CameraPose`. This persists even when the actual CameraDirector/CameraContext is active. In contrast, the camera matrix getters at lines314–358 already prefer the original context.

Concrete differences:

- `calcNormalizedScreenPositionFromView` at line665 assumes positive view-space Z is in front, returns raw view-space Z, and rebuilds focal terms from pose FOV/aspect. The original at `decomp/src/Game/Util/CameraUtil.cpp:90–103` multiplies the actual projection, performs the homogeneous divide, flips Y, and returns projected Z. The original perspective matrix has W = -viewZ. Its visibility result checks X/Y against ±1 and projected Z against zero. The compat helper ignores the actual projection offsets here.
- `project_world_to_screen` returns only a near/far depth test. Original `calcScreenPosition` preserves the normalized helper's X/Y visibility result as well. Off-screen points can therefore receive different boolean results.
- Pixel conversion hardcodes `kWiiLogicalFramebufferWidth/Height`, while the original queries `MR::getScreenWidth/Height`. The already present original CameraContext fixture establishes console width608 versus832 for4:3 versus16:9.
- Projection/unprojection and distance helpers can use a cached pose while `MR::getCameraViewMtx` exposes the current actual view. This creates two authorities for camera queries within a frame.

Retail confirmation: `CameraUtil.s:803C8594–803C86CC` reads the real CameraContext projection, divides by -viewZ, flips Y, tests both normalized axes, then tests projected Z. `803C838C–803C8444` queries screen dimensions. This supports the source semantics without relying on the decompiler's comments.

**Minimal integration:** use the existing original projection/unprojection/distance wrapper cohort (`decomp CameraUtil.cpp:56–145`) with the actual scene CameraContext. Then remove private host projection arithmetic and pose fallback for those MR functions. The real original CameraDirector's controller and interpolation logic are already active and need no rewrite. Enable the whole CameraUtil TU only when its duplicate symbols across `CameraUtilCompat.cpp` and `OriginalCameraOwnerUtil.cpp` are removed coherently. Host resource retention/native owner registration remain appropriate compatibility responsibilities.

**Validation needed:** expand `OriginalCameraContextTests` or `OriginalCameraDirectorTests` with actual MR world/view projection results for a negative-Z visible point, off-screen point, near/far projected depth, shake offsets, both console aspects and projection/unprojection round trip. Check that queries observe a changed actual context without requiring RuntimeContext pose publication first. `CameraUtilRealOrAbsentTests` checks only missing state and cannot establish these semantics. Run the same actual camera/jump replay afterwards.

**Match status:** original CameraUtil is present but marked `NonMatching` in decomp configure.py; its projection helpers include code-generation FIXME comments. No fresh objdiff was run. The exact control flow and matrix convention above were verified against retail.

## 3. Remove duplicate original Player accessor providers from the archive

This is the smallest maintenance cleanup and can precede the semantic changes above. `llvm-nm -A -C --defined-only build/macosx/arm64/debug/libsmg-pc-game.a` confirmed strong duplicate providers in the actual archive:

| Original object already present | Compatibility object still present | Duplicates observed |
| --- | --- | --- |
| `MarioAccess.cpp.o` | `MarioCameraAccessCompat.cpp.o` | `MarioAccess::isOnGround`, `getPlayerActor`; the file also copies the surrounding camera-state accessor family. |
| `MarioActorGravity.cpp.o` | `MarioStateAccessCompat.cpp.o` | `MarioActor::getGravityVec`, `getGravityVector(TVec3f*)`. |
| `MarioActorGravity.cpp.o` | `MarioCameraAccessCompat.cpp.o` | Source also defines `MarioActor::getGravityInfo` in both. |

The compatibility file's comment claiming full MarioAccess/MarioActorGravity are not enabled is obsolete. Archive extraction/dead stripping currently allows the demo to link; this audit did not identify which equivalent duplicate body was selected for every final executable. Reliance on extraction order is unnecessary and complicates subsequent original TU activation.

**Minimal integration:** remove the duplicate MarioAccess and MarioActorGravity bodies from these compatibility slices, keeping each symbol in its actual original TU. Do not blindly delete both entire files: MarioCameraAccessCompat also provides MR PlayerUtil wrappers, while `MarioStateAccessCompat` owns `Mario::{getCurrentStatus,isStatusActive}` because full `Player/MarioState.cpp` is still excluded. Migrate those remaining groups only with their original owner activation.

**Validation needed:** re-list archive symbols to prove a single intended original provider, link the actual showcase plus original camera/player fixtures, and re-run the working movement/jump replay. No new gameplay algorithm or synthetic fixture is needed for removal of identical accessor bodies.

## 4. Follow-up consolidation, with explicit prerequisites

- **CameraLocalUtil/CameraTargetObj:** decomp contains both original TUs; neither source currently exists in root Game/Camera. CameraLocalUtilRuntime and OriginalCameraOwnerUtil split copied bodies, with TLS target/mode bindings for the older standalone controller lane. The live path already uses actual `CameraMan::mDirector` when present. A later import can remove those copied bodies and the TLS fallback after migrating old controller fixtures to the real director owner. Historical source-correspondence notes: `notes/original-camera-local-runtime-20260903T035134Z/README.md` and `notes/original-camera-holder-activation-20260907/original-helper-manifest.json`. Both decomp TUs are marked NonMatching; no new full-TU proof is claimed.
- **Old CameraSystemService pipeline:** `RuntimeServices.cpp:2739` returns immediately from begin_frame when the actual CameraDirector runtime exists. The remaining custom game/event/view/shake orchestration plus `src/camera/OriginalCameraView.cpp` is a second controller lifecycle still used by old fixtures/manual camera APIs. It is a removal candidate after identifying every remaining non-original-owner caller, not evidence that the working demo uses a fake camera. Keep native CameraPose publication as a renderer-facing snapshot of actual CameraContext (`CameraDirectorRuntime.cpp:54–69`).
- **GravityUtil:** the excluded original TU has the same manager query and JMap argument algorithms currently copied by GameGravityCompat. Real PlanetGravityManager already performs the physics. Consolidation is plausible, but preserve the native pointer-to-u32 conversion contract and missing scene/manager checks. `calcGravityOrZero` at the end of GameGravityCompat originates outside GravityUtil and must not be dropped blindly. This is lower priority than the source-confirmed player/camera differences above.
- **Input:** separate audit by the audio agent is `input-audit.md` in this directory. Do not simply replace GamePadUtilCompat with its original TU: native Nunchuk data publication and original WPad record updates still have documented prerequisites, including a missing assignment in the existing decomp WPadStick update. Preserve the now-proven WASD path until those are addressed and tested.

## Scope/proof limits

Only this note was written. Existing source/retail assembly and the already-built Game archive were read; no new binary or runtime claim was produced. The strongest immediate opportunities are the player MR authority seam, original camera projection semantics, and removal of duplicate original Player accessor providers. No change to Mario movement/camera controller algorithms is proposed by this audit.
