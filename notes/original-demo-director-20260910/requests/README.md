# Original demo start/request utility recovery — 2026-09-10

Complete `DemoStartRequestUtil.cpp` recovered in decomp first and mirrored byte-identically into native. The previous file contained only `isEmpty`; this adds 17 public methods and one local common initializer. Existing complete typed declarations already agreed with the retail entry points, so neither header was edited. Source SHA256: `186d7a6b4cd5fe0b2d5471958e1ee795a96547bc56db218495984203f731e269`.

## Original contracts retained

* Immediate requests call `MR::canStartDemo`, select the optional start nerve first, then perform the original scene-stop/director/HUD/pointer/movement/camera/effect/Mario/broadcast sequence. Programmable LiveActor and LayoutActor requests explicitly resume registered effects; the NerveExecutor/timekeep overloads have their original distinct paths.
* Deferred requests select the optional waiting nerve, construct the actual `DemoStartInfo`, retain its requester in the matching typed field and its starting nerve, resolve the actual executor by demo name, register the record, and queue the corresponding typed identity. They return false; that return does not mean the request was discarded.
* NerveExecutor requests retain their separate LiveActor as the `_10` NameObj movement companion. Starter precedence is LiveActor, LayoutActor, NameObj, then NerveExecutor's companion. Nerve precedence excludes NameObj and favors LiveActor, LayoutActor, then NerveExecutor. Movement resumes all populated actor/object/companion fields independently.
* Queued start does not simply reuse the immediate start routine: it restores movement/nerve state in retail order, uses the director's actual proxy for a timekeep record without a LiveActor, then invokes the stored executor path including the optional part name. Both executor calls present in retail are retained; no deduplication or host routing substitution was introduced.
* The actual holder supplies FIFO/capacity/record lifetime behavior. `startDemo(holder)` re-reads the current info after its null check; pop returns false for no current info and otherwise consumes exactly one request. Borrowed names, nerves, actors, and executor pointers remain borrowed as in the original.

## Proof

Fresh Wii full-TU compilation passes before and after. All 19 functions are present; 17 match100%, the main immediate start is96.21795%, and queued start is97.37374%. The remaining differences are equivalent boolean code generation/register assignment and the existing unsigned record fields; no known call or control-flow mismatch remains. The immediate predicate folds equality against2/3 to an unsigned range, while the retail uses two equality comparisons. The values and behavior agree for all s32 inputs.

All 3212 instruction bytes in the 19 saved retail function bodies match the actual DOL (`25c5959534b3c21246c6c7e42021b916b41fb578`). See `retail-byte-proof.json`, `objdiff-summary.json`, full objdiff JSON, compiler commands/logs, source manifest and reference patch. Isolated native compilation passed with only the then-missing exact original DemoDirector header supplied from a notes overlay; parent is importing that header and original director implementation. This compile is not linked demo runtime proof.

## Native integration finding

The native ObjUtil header declared only `MR::requestMovementOn(NameObj*)`, so overload resolution silently converted LiveActor/LayoutActor calls to NameObj. The original LiveActor overload also resumes its actual effect keeper. This concrete closure gap was reported to parent, which owns restoration of the two original overload declarations/providers. No alternative shim or Game source adaptation was added here. `native-undefined.txt` records the pre-restoration native import set and must not be interpreted as proof that the missing overload semantics existed.

`DemoTimeKeeper::isDemoEnd` was independently reviewed on parent request and is correct as recovered. See `timekeeper-review.md` and its direct 108-byte DOL proof; no TimeKeeper source was changed.

Parent owns the shared build, real original director fixture, provider retirement, and commits. No global Xmake, runtime, or commit was performed by this subtask.
