# Original auto-effect registration — 2026-09-03

Root-first recovery is frozen. No native production source, build selection, scene owner, or gameplay path changed. Parent owns the checkpoint and later atomic effect-system activation.

## Root checkpoint

- `src/Game/Effect/EffectSystemUtil.cpp`: common metadata setup; actual ModelManager-to-XanimePlayer delegates; live-actor setup/BCK synchronization; four group-registration overloads; three addAutoEffect overloads; movement-unpause traversal.
- `include/Game/Effect/AutoEffectInfo.hpp` and `src/Game/Effect/AutoEffectInfo.cpp`: type the actual offset record at 0x2C as `TVec3f mOffset`, with the same x/y/z storage and parser assignments. This avoids treating three separately declared floats as a native TVec3 object when retail directly passes record+0x2C to `setOffset`.
- `include/Game/Effect/MultiEmitter.hpp`: `_28` is the retained `const AutoEffectInfo*` received by registration; consumers only read it. No pointer storage/layout change.

The parent's four previously recovered particle lifecycle/link methods remain unchanged and still match all 180 relocated instruction bytes. `root.patch` is the complete four-file source delta and passes reverse application against the written checkpoint. `root-manifest.json` identifies the exact source hashes. No new source placeholders or empty providers were added.

## Original semantics retained

The common setup retains the actual metadata pointer, sets authored draw order and live offset, changes scale/rate only when they differ from 1 by the original `isNearZero` tolerance, applies RGB colors only when flagged valid, and changes light influence only when not near zero. The original constants are 1, 0, -1 and 0.001 (0x3A83126F); no PC tuning factors are introduced.

Live actors register at an authored joint when present, otherwise use their original virtual base matrix plus independent scale, otherwise retained position/rotation/scale pointers. Both original base-matrix virtual calls remain. The effect name is captured before the original matrix/joint call, preserving evaluation order visible in retail. Layout and multiscene overloads call their actual keeper APIs. The live setup then initializes BCK synchronization, applies metadata, scans the real EffectSystem, and links an authored parent emitter.

BCK synchronization returns immediately for a null animation name. One name uses the authored start/end frames. A space-containing list uses the original 256-byte temporary, counts space/NUL delimiters including empty segments, resolves each token through the model's actual animation ResourceHolder, registers the first with frame range 0/-1 and appends subsequent names. It preserves the original lookup-failure behavior; there is no fabricated animation name or skipped-token recovery. The original buffer requires tokens to fit its 255-character payload before any future native activation. ContinueAnimEnd uses flag 0x40. Group creation and registration remain distinct operations, including the original two scene-system lookups in the live-actor overload.

## Retail/compiler proof

`verify-root.py` compiles the actual complete TU with unchanged GC3.0a3 Game flags and compares the extracted object with SHA1-verified RMGK01 retail DOL `25c5959534b3c21246c6c7e42021b916b41fb578`.

The 13 newly restored functions span 1,996 retail bytes. Twelve functions (1,420 bytes) relocate to exactly identical instructions. Eleven score 100% in objdiff; common metadata setup scores 99.78261% only because of compiler constant-label identity and is also relocated-byte exact. `setupMultiEmitterSyncBck` scores 95.24306% (576 retail / 564 rebuilt bytes). Its **141 canonical instructions** match after explicit, checked temporary-register allocation changes and equivalent extraction of the 0x40 flag. All calls, field offsets, argument values and branch destinations remain in the comparison. The equivalence of the two boolean sequences is checked for all 65,536 metadata flag values. The shared external retail f64 integer-conversion constant at 0x80531A90 is verified against its actual 0x4330000080000000 bytes before relocation.

The full previous/current AutoEffectInfo constructor/parser/helper object instructions and relocation identities are also equal; only generated literal labels are normalized by exact bytes. Thirteen shared code/data symbols are checked, including the complete constructor, parser, accessors and draw-order table. The stored baseline metadata source/header allow this proof to be reproduced after commit. Existing particle methods are included in the same proof so later recovery cannot silently change them.

Commands and results are in `root-evidence.json`, `root-proof.log`, and the annotated disassemblies. Reproduce with `python3 pc-port/notes/original-auto-effect-registration-20260903/verify-root.py`.

## Native preparation and tested limit

`stage.py` places complete unchanged root files and the actual compile-dependency headers under `build/original-auto-effect-registration-20260903/staged`. Complete EffectSystemUtil and AutoEffectInfo TUs compile with the current native/Aurora/root include order. Header overlays are preparation for coherent activation, not permission to replace a live keeper's layout without its implementation. The exact file list is in `native-manifest.json`; `native-undefined.txt` reports the full real link boundary.

The updated native metadata probe executes actual AutoEffectGroup allocation/add and AutoEffectInfo parsing against extracted retail Effect.arc: **2,591 records, 614 groups, 170 authored colors and 1,024 RGBA round trips pass ASan/UBSan**. It includes the typed offset components. This requires the already documented general Color8 integer-endian correction from `original-auto-effect-20260903/color-native.patch`; without that still-unapplied correction the native authored-color check fails. The same patch is staged here, with no new color semantics. Existing native archives are not sanitizer-instrumented; the newly compiled original metadata/group/probe TUs are.

The metadata fixture does not call registration overloads or publish an EffectSystem. No fake keepers, model casts, name predicates, resource holders, or missing-body stubs are used to link them. `verify-native.py` reproduces this exact bounded compile/metadata validation.

The next actual source prerequisites are `MultiEmitter::scanParticleEmitter`, `initSyncBck`, `onDeleteSyncBck`, `addSyncBck`, and `setContinueBckEnd`, currently root placeholders, plus real SyncBck/keeper/emitter/callback/system ownership. Existing root `MR::hasStringSpace` and `findBckNameStringInResource` can be imported literally when needed; the latter queries the genuine motion resource table. Remaining EffectSystemUtil APIs such as createAutoEffect/attribute lookup are separate missing functions. No native registration or MarioEffect activation is claimed by this checkpoint.
