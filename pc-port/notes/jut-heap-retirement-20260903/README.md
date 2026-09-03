# Native JUTTexture retirement with original JKR heaps

This staged change closes a native resource-lifetime mismatch. Original scene heaps can reclaim a JUTTexture wrapper without explicitly calling its destructor. Native JUTTexture storage is independently mapped into MEM1 and retained by a host registry, so merely reclaiming the Game wrapper leaks that mapped lease and GX resource identities.

The generic `JkrHeapFinalizer` registry associates a completely constructed object with the actual heap returned by `JKRHeap::findFromRoot(object)`. It does not change JUTTexture layout, allocate a fake disposer in a Game heap, retain a heap/domain cycle, or special-case any actor or texture name. Host/stack objects remain explicitly owned.

Native JKRHeap `callAllDisposer`, complete `dispose`, and range disposal run remaining native finalizers after the original disposer loop. This allows genuine owning disposers to delete a texture first, while ensuring any remaining native attachment retires before original freeAll/freeTail/domain destruction reuse its wrapper storage. Native metadata does not change the original heap algorithms. Explicit raw `free` retains the original SDK requirement that the caller first destroy constructed objects; the adapter does not redefine raw free as C++ delete.

All three JUT constructors register only after successful initialization. The owned constructor keeps its existing rollback allocation guard until registration succeeds. Borrowed registration failure retires its initialized GX object before rethrowing. The destructor first unregisters, then executes existing GX destruction and mapped lease release. Bulk callbacks invoke that same destructor. A texture owned by a host enclosing object keeps its existing explicit cleanup path.

Retirement extracts one record, unlocks, invokes the callback, then rescans. A callback may delete/unregister another object without leaving an invalid iterator or double callback. Teardown makes no allocation and exposes only noexcept callbacks. The synchronization owner is created during registration; an atomic empty count avoids constructing it for empty teardown or a host-only destructor. Registry locks are released before existing `GXDestroyTexObj`, `GXDestroyCopyTex`, and MEM1 allocation release drain queued GX CPU reads. Actual scene owners still must retire drawing/scheduler users before freeing their original heap, and clear their borrowed selected-texture pointers as appropriate.

## Evidence and application status

Production was not edited. Apply-ready files and narrow patch are staged under `build/jut-heap-retirement-20260903/`; durable `native.patch` and `native-files.json` are included here. Parent owns application, target wiring, shared build, GPU run and checkpoint.

`python3 build/jut-heap-retirement-20260903/verify-isolated.py` compiles the registry, modified original-heap native provider, JUT constructor/destructor adapter, CPU fixture, and expanded texture fixture using actual current target flags. It links and runs only the CPU fixture against the real existing SDK/Game archives. All five source files compile. The CPU run passes head/bulk reclamation, tail-only reclamation, exact range disposal, domain destruction, host exemption, explicit delete, original owning disposer ordering, and a callback deleting another registered object. The same test proves later heap reuse remains valid.

The expanded existing `JutTextureOwnershipTests.cpp` compiles and is ready for the parent's GPU slot. It retains every prior assertion and adds actual mapped texture head/tail/bulk/domain retirement, queued capture before retirement, explicit-delete-then-bulk behavior, borrowed TIMG nonownership, constructor OOM, and original domain survival after allocation-provider retirement. **GPU execution is pending**, so this note does not claim that runtime gate passed.

Suggested CPU target name: `smg-pc-jkr-heap-finalizer-tests`, using the same dependency/compat.cpp list as existing original JMap heap-lifetime fixtures. The modified existing GPU target is `smg-pc-jut-texture-ownership-tests`. No production test-only hooks are needed.

## Integrated runtime result

Applied to production and built with LLVM23 on arm64 macOS. `smg-pc-original-jkr-heap-finalizer-tests` passes every CPU lifecycle assertion. `smg-pc-jut-texture-ownership-tests` passes on Aurora Metal/Apple M5 Max, including queued captures followed by real original heap cleanup, allocation failure, provider retirement and restoration of full MEM1 capacity. The original owning-constructor rollback guard already destroys its initialized GX identity if finalizer registration fails; the borrowed constructor has its own rollback.

Reproduce from pc-port with `xmake build smg-pc-original-jkr-heap-finalizer-tests` and `xmake build smg-pc-jut-texture-ownership-tests`, then run the matching binaries under build/macosx/arm64/debug. Source and tests are committed; generated build and full runtime logs remain local.
