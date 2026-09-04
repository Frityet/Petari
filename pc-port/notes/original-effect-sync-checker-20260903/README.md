# Original effect animation synchronization — 2026-09-03

Root source is frozen. No native production source, build selection, runtime owner, or gameplay path changed. The parent handles checkpointing and eventual coherent effect-system activation.

## Source checkpoint

- `src/Game/Effect/SyncBckEffectChecker.cpp`: all seven missing methods recovered; the existing `reset` is unchanged. The complete original checker now exists.
- `src/Game/Effect/MultiEmitter.cpp`: recover original `setDrawOrder` using array traversal and a by-value `u8` member-function binder. It delegates each element to the existing original `SingleEmitter::setGroupID`; it does not clamp authored groups or recurse into child emitters.
- `src/Game/LiveActor/EffectKeeper.cpp`: correct two coupled decomp errors in `syncEffectBck`. Continuous creation is gated by its independent `b2` result, and attribute-remapped creation uses a separate local target. Deletion continues to use the original input emitter, exactly as retail does.

No original class layout or public API changed, and no stub, missing-body replacement, or invented owner was introduced. The root/native patch files and source hashes describe this exact delta. Native activation also needs the previous MultiEmitter/SyncBckEffectInfo package and real keeper/particle ownership; the patch is preparation, not standalone owner activation.

## Original timing semantics

`updateBefore` checks termination on the actual active `_24[_54]` frame-controller bank, while rate/frame come from the selected `_20` controller. A zero-rate animation still supplies a current BCK if its frame changed externally. A terminated or unchanged zero-rate frame supplies null. This is the original split during transitions, not a substituted animation-state predicate.

`updateAfter` clears reset, stores current name identity as previous, and snapshots the selected controller's frame. `reset` changes only reset/frame history; it preserves both name pointers. The checker constructor begins with reset false.

Continuous creation is eligible whenever the current BCK is registered. One-time creation uses the original start-frame crossing and a first-frame correction at `start + rate - current`, with the original 0.001 tolerance and only for nonzero rates. The redundant second registration test remains because it is in retail.

At nonzero rates, crossing delegates to the actual XanimePlayer/J3DFrameCtrl logic, including XanimePlayer's retained pre-update frame. At zero rate, it uses the previous/current half-open interval. Only attribute 2 treats a backwards frame change as a forward loop wrap, splitting at the actual end/loop values. Other modes use ordinary forward/reverse intervals. No modulo, frame-rate multiplier, or smoothing is added.

Deletion preserves the original identity comparisons, continuation flag, current-name lookup, named termination query, and explicit end-frame test. The seemingly redundant registered/loop branch is retained. The native fixture does not fabricate a resource object to force an unreachable branch.

## Compiler and retail proof

The original GC3.0a3 compiler checks **10 methods / 1,440 bytes**, including the already-completed reset. Nine methods / 1,404 bytes relocate byte-exact to verified RMGK01 DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`.

`updateAfter` is 97.77778% and differs only in four instructions' temporary register choices. The proof checks the complete nine-instruction stream against exact expected word differences: copied-name uses r0 instead of r5, and zero uses r4 instead of r0 before r4 is reused for the player pointer. Stores, fields, order, and values are unchanged. Seven of the other methods score 100%; stopped-frame crossing is 99.672134% because the compiler emits its own equivalent signed-integer conversion constant, and draw order is 99.72973% because of a generated member-function descriptor label. Both are relocated-byte exact.

The proof validates the shared f64 constant at 0x80531BD0, the complete member-function descriptor and target, and the keeper's actual relocated `Attr` string pointer. All **89 previously compiled methods** across these TUs remain unchanged except the intentionally corrected keeper method. This preservation check does not assert retail correctness for unrelated older decomps.

```sh
python3 pc-port/notes/original-effect-sync-checker-20260903/verify-root.py
```

`root-evidence.json`, `root-proof.log`, captured baselines, and annotated DOL disassemblies retain the evidence. `root.patch` passes reverse-application against the recovered source.

## Native compilation and CPU validation

`stage.py` prepares complete original MultiEmitter, SyncBckEffectInfo, SyncBckEffectChecker and EffectKeeper TUs with real dependency headers. All four TUs compile unchanged under native flags. `native-manifest.json` records source hashes; `native-undefined.txt` records the remaining actual link boundary.

Three isolated test groups pass ASan/UBSan using a genuine binary-backed ResourceArchiveOwner, original BCK loader, intentionally one-joint test model, actual J3DModel constructor and actual XanimePlayer:

- Five real player update/calc phases produce exactly one start crossing and one end crossing, with continuous eligibility and the original reset tolerance.
- Twenty-three literal stopped-frame boundary cases cover forward/reverse movement, equal frames, attribute-2 wrap, mirror mode, negative values and unknown mode behavior. Additional cases exercise original nonzero forward-wrap/reverse dispatch.
- Animation changes, stopped frames, continuation, absent/terminated animation, active-versus-selected controller banks, and reset's retained name identities follow the recovered rules.

The bank case sets public fields on the actual test-owned SDK frame controllers to exercise their distinct roles. No test-only production API or replacement Game implementation is used. Newly compiled checker/info/probe TUs are sanitizer-instrumented; pre-existing native archives are not. JKR allocations use a real retained cohort, and native test output is outside Game allocation routing.

```sh
python3 pc-port/notes/original-effect-sync-checker-20260903/verify-native.py
```

The fixture executes original player/checker behavior, not a fabricated emitter. Draw-order and keeper corrections have full compiler/retail proof; actual runtime emitter creation/deletion still requires genuine EffectSystem/JPA manager/keeper/callback ownership. This checkpoint does not claim activated native effects or completed jumping.
