# Original pre-draw scheduler boundary

Frozen staged package, revised after allocation-routing review; see `REVIEW_CORRECTION.md`. No production native activation, shared build, GPU run, or RuntimeContext edit was performed here.

## Result

The existing `SceneDrawBufferService` now owns an actual `NameObjListExecutor`. That executor owns its actual `NameObjCategoryList` and the service's existing actual `DrawBufferHolder`. This extends the existing scheduler owner instead of introducing another scene executor. Only the draw list is initialized by this boundary; movement and calc ownership remain in the existing native scheduler.

The original `NameObjListExecutor::registerPreDrawFunction` calls the original `NameObjCategoryList::registerExecuteBeforeFunction`, which clones the real `MR::FunctorBase`. The original `executeDraw` calls the original category `execute`: an empty list returns before the callback; a nonempty list calls its callback once before invoking each object's original draw delegator. The two complete source files are literal root copies, including root's recovered swap-last remove algorithm.

Native `MR::registerPreDrawFunction` resolves the existing active scheduler binding, then RuntimeContext's existing scheduler owner. Missing owner is an explicit construction error. The Wii GameSystem/SceneController singleton route is not fabricated. Its existing literal root body was separately compiled and matched 100%.

## Source evidence

- `root.patch`, `root/`, and `ROOT_CHECKPOINT.md`: the only recovered root method in this tranche is `NameObjCategoryList::remove` at 0x80261E78, size 0x6C. All 15 category text functions and both delegator vtables match the original compiler 100%.
- Existing `NameObjListExecutor` forwarding methods match 100%; its existing destructor remains 98.41% due the existing inline draw-holder decompilation.
- `OriginalPreDrawRegistration-root.cpp` and `mr-registration-evidence.json`: literal existing MR wrapper, 100% original compiler match at 0x803F10F0, size 0x4C.
- `dol-evidence.json`: all 47 selected retail text functions match the DOL after masking relocations. DOL SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`.
- `scene/DrawCategoryInitialTable.inc` is extracted from root `SceneNameObjListExecutor.cpp::cDrawListInitTable`, including its sentinel. `std::size(table)-1` supplies all 83 categories. Only the unsigned -1 sentinel spelling changes for the host compiler.

## Native ownership and portability

1. The original category arrays are real `MR::Vector<MR::AssignableArray<NameObj*>>` storage. The host membership adapter grows their typed pointer allocation when the existing dynamic scheduler discovers new objects. It calls original remove/add and executes the original list; it does not implement a second callback or object draw loop. Removal preserves the original swap-last behavior.
2. Both original delegator template instances derive from a native common virtual base. The Wii header stores const and non-const template pointers in a union and invokes/deletes through the non-const pointer. Those are unrelated dynamic types in native C++; the common base makes the same original calls well-defined. The root header and Wii ABI are unchanged.
3. Callback history owns actual cloned functors and retains their originating `JkrAllocationDomain`. Replacements are retained until rollback or clear, because an older registration may need restoration. A currently executing callback holds an additional reference through completion. Host container/control-block allocation is independent of Game heaps. Host allocation scopes end before original pre-draw/object invocation and before the legacy effect draw call, preserving the caller's current heap and routing. Invocation does not enter the callback registration domain. A clone's destructor runs before its retained domain is released.
4. The registration marker includes callback registrations. Scoped rollback removes scoped objects and callback versions and restores the prior callback. `clear()` detaches category members before destroying layout adapters and releases all registrations. A NameObj destructor consults the active scheduler binding before the existing runtime scheduler; isolated construction/destruction uses the matching binding.
5. Custom native `LayoutRuntime` instances have owned, actual `NameObj` draw adapters. They participate in the same category batch as Game NameObjs, forwarding to the existing real `LayoutRuntime::draw`. The adapter is removed from the original list before destruction.
6. Functor header additions preserve the native virtual-destructor ABI and support actual function and const-member functors. The generic clone uses the requested JKR heap. This is the same prerequisite header package as the preceding particle-draw checkpoint.
7. The actual model buffer owner retains its existing `GXDrawDone` teardown fence. CPU category-only ownership does not create a GPU fence. The initial probe exposed an unconditional fence hanging against an unstarted FIFO; `superseded-cpu-teardown-hang.sample` preserves that finding. No GX provider was replaced. GPU/model teardown was not exercised by this CPU fixture.

Recursive execution of the same category is rejected before rebuilding its active pointer batch. Callback replacement, rollback, and clear during the pre-draw callback are supported. General arbitrary object deletion during the original object-draw iteration is not claimed safe; the original iterator algorithm and its caller lifetime contract remain unchanged. Scheduler mutation is single-threaded.

## CPU validation

`native-compiles.json` and `native-verification.json` contain the exact isolated LLVM 23 commands. All six complete provider TUs plus the probe were compiled with ASan and UBSan, and linked against the existing prebuilt native libraries. Those prebuilt libraries are not newly sanitizer-instrumented by this task.

`probe-runtime.log` records a successful process exit with no sanitizer diagnostics. Assertions cover:

- original insertion order and swap-last removal;
- exactly one pre-draw call for a nonempty batch and none for an empty batch;
- actual free-function and const-member Functor cloning and dispatch;
- callback replacement while the old callback is executing;
- callback/NameObj scene rollback and automatic NameObj destructor removal;
- real JKR child-heap retention until rollback;
- ordinary allocations from an actual pre-draw functor and NameObj draw belonging to the caller's selected invocation heap, distinct from the callback registration heap;
- host-owned metadata surviving invocation-heap retirement and overwritten heap reuse, with later host invocation still using host allocation routing;
- clear during pre-draw, including proof that the executing clone is destroyed only after return;
- original table bounds (category 83 is rejected);
- a mixed NameObj/LayoutRuntime batch, which reaches the real LayoutRuntime renderer boundary after the callback and preceding object.

The layout check intentionally observes `Aurora renderer context is not active`; it does not claim layout pixels or GPU rendering. No original particle draw callback was executed on a GPU.

Reproduce:

```
python3 pc-port/notes/original-predraw-scheduler-20260903/verify-root.py
python3 pc-port/notes/original-predraw-scheduler-20260903/verify-mr.py
python3 pc-port/notes/original-predraw-scheduler-20260903/verify-dol.py
python3 pc-port/notes/original-predraw-scheduler-20260903/stage.py
python3 pc-port/notes/original-predraw-scheduler-20260903/verify-native.py
```

## Apply and provider inventory

`native.patch` covers 14 production paths; `native/` contains their exact final copies and `native-manifest.json` records base/final hashes. The patch passed `git apply --check` at freeze time. It includes the prior draw checkpoint's Functor/ObjUtil header additions, so do not separately overwrite those headers afterward. All native work is unapplied.

```
git apply --check pc-port/notes/original-predraw-scheduler-20260903/native.patch
git apply pc-port/notes/original-predraw-scheduler-20260903/native.patch
```

New providers to select in the parent's build are:

- `src/Game/NameObj/NameObjCategoryList.cpp`: complete original category implementation.
- `src/Game/NameObj/NameObjListExecutor.cpp`: complete original executor implementation.
- `src/compat/OriginalPreDrawRegistration.cpp`: single native MR owner-lookup provider.

The existing SceneScheduler, SceneDrawBufferService, and NameObj providers are replaced in place. There was no native MR pre-draw definition to remove. Do not add a second NameObjListExecutor owner or a second pre-draw loop. RuntimeContext requires no new member.

Actual EffectSystem/ParticleDrawExecutor ownership and category registrations must still be activated coherently by the parent. The pre-existing `runtime->effects().draw(...)` call at the end of native `execute_draw_type` belongs to the legacy effect service and remains unchanged in this isolated patch; retire that call with the actual original effect owner so particles are not submitted twice. The current CPU proof establishes the scheduler prerequisite, not full effect activation.
