# Original MultiEmitter / SyncBckEffectInfo — 2026-09-03

Root recovery is frozen. No native production source, shared build selection, scene binding, or gameplay path changed. The parent owns checkpointing and later effect-system activation.

## Recovered source

- `src/Game/Effect/MultiEmitter.cpp`: five original methods — particle classification scan, sync-record creation, delete-frame assignment, additional BCK registration, continuation assignment.
- New `include/Game/Effect/SyncBckEffectInfo.hpp` and `src/Game/Effect/SyncBckEffectInfo.cpp`: complete seven-method resource metadata class and original delete-frame predicate. The 24-byte Wii object contains an actual pointer vector at 0, start/end frames at 0xC/0x10, and continuation at 0x14. Each 8-byte BCK record retains the passed name and the actual `J3DAnmTransform*` returned by the player's original resource table.

The scan traverses only this emitter's actual SingleEmitter array using the original `for_each_array`/`bind2nd` member-function wrapper. SingleEmitter's existing original scan temporarily creates a real particle emitter to classify continuous versus one-time emission, then deletes that temporary. No name-based classification or synthetic EffectSystem was introduced.

`initSyncBck` constructs the original info with start 0, end -1, continuation false, then assigns the requested start frame. Retail `onDeleteSyncBck(bool, f32)` ignores its boolean argument and only assigns the end frame. `addSyncBck` directly delegates to the retained info; continuation directly assigns its bool. No new null guards or replacement semantics are added to original functions.

The info constructor allocates the authored pointer capacity and adds its first BCK. Name queries use original **case-insensitive** `MR::isEqualStringCase`; null/missing queries return false, and the first matching duplicate wins. Only original resource attributes 2 and 4 count as looping. End-frame presence is exactly `0 <= endFrame`, including true for either signed zero and positive infinity, false for negative values and unordered NaN. Caller capacity and resource/name lifetimes remain original preconditions: registration does not grow the vector or own copied names, and `isLoop` expects a successfully resolved animation.

## Retail/compiler evidence

All **12 restored methods / 980 bytes relocate byte-exact** to SHA1-verified RMGK01 DOL `25c5959534b3c21246c6c7e42021b916b41fb578`. Eleven score 100% in objdiff. The scan scores 99.72222% solely because the generated member-function literal label differs; the actual instructions and the complete 12-byte descriptor at 0x80578588, including the `SingleEmitter::scanParticleEmitter` relocation, match exactly. No instruction normalization is used for these claims.

All **40 previously compiled MultiEmitter methods** retain identical code and relocation identities against the captured source baseline. This is a preservation check, not a claim that those earlier decomps all match retail; existing unrelated discrepancies are unchanged. The original class size and field offsets are checked by `layout-original.cpp` with GC3.0a3.

Reproduce:

```sh
python3 pc-port/notes/original-multi-emitter-sync-20260903/verify-root.py
```

`root-evidence.json` records exact commands, hashes, addresses, sizes and relocated targets. `root-proof.log`, the annotated disassemblies and `root.patch` are the checkpoint evidence. The stored baseline makes preservation verification independent of later commits.

## Native preparation and bounded validation

`stage.py` copies complete root MultiEmitter and SyncBckEffectInfo TUs plus their real dependency headers under `build/original-multi-emitter-sync-20260903/staged`. Both complete TUs compile without source changes under current native flags. The new pointer vector and all `new` expressions use the actual native types and sizes. The manifest records every overlay; `native-undefined.txt` records the remaining linkage boundary. `native.patch` contains only the three recovered native source/header imports and remains unapplied.

The isolated ASan/UBSan test constructs a real ResourceArchiveOwner from a valid RARC containing three binary BCK fixtures. The original loader produces actual J3DAnmTransformKey objects. A real deliberately joint-only test model, actual J3DModel constructor and actual XanimePlayer supply the original resource lookup chain. This is not an incomplete archive model published as a full BMD, and it does not fabricate a ResourceHolder or player layout.

The test passes construction/capacity/frame retention, exact loaded-resource identity, case-insensitive lookup, null/missing names, duplicate ordering, all 256 attribute values, nine frame boundaries, and archive ownership through consumer teardown. Allocations use a genuine retained JKR domain; original pointer-array destruction and final heap retirement reclaim their respective storage. It uses no replacement implementation of a missing Game method. Newly compiled sync/probe TUs are sanitizer-instrumented; pre-existing linked native archives are not.

```sh
python3 pc-port/notes/original-multi-emitter-sync-20260903/verify-native.py
```

The test executes SyncBckEffectInfo's real methods. MultiEmitter's new methods have complete compiler/retail proof but are **not** runtime-tested through a fabricated emitter instance. Running its scan still requires genuine EffectSystem/particle manager/callback ownership. The actual SyncBckEffectChecker remains mostly missing in root, and `MultiEmitter::setDrawOrder` is still a root placeholder. Existing EffectSystemUtil resource-name lookup and keeper/callback/system closure are further prerequisites. No native effects or MarioEffect activation is claimed.
