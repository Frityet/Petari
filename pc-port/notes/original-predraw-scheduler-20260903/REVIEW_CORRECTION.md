# Preserve the caller's Game allocation routing

Parent review found that the previous staging held `JkrHostAllocationScope` across both `SceneScheduler::execute_draw_type` and `SceneDrawBufferService::execute_draw_category`. This incorrectly disabled Game allocation routing for original pre-draw/object calls. The earlier CPU fixture did not test invocation allocations, so its passing result did not cover this behavior.

The corrected scopes cover only entry/batch metadata assembly, original pointer-array growth, trace construction, and result copying. Original `NameObjListExecutor::executeDraw` and the existing legacy effect draw call execute after those host scopes end. Neither path enters the callback registration domain; the caller's current heap and routing remain authoritative.

`verify_invocation_allocation_routing` registers an actual const-member `MR::Functor` in one real JKR child heap, then calls the original category dispatch under another real child heap. Both its pre-draw method and its NameObj draw override allocate ordinary `new[]` storage. `JKRHeap::findFromRoot` confirms both allocations belong to the invocation heap. A new allocation after dispatch confirms caller routing remains active. The trace vector is verified to belong to no JKR heap.

The invocation heap is then released and expires, and a child allocation is reused and overwritten before scheduler metadata is inspected and dispatched again. Both calls now allocate on the host, despite the retained registration heap. This checks metadata lifetime and confirms invocation does not force the registration domain.

The complete six-TU ASan/UBSan fixture passes with the new `invocation_heap_routing` and `metadata_after_heap_retirement` assertions, alongside prior callback/order/lifetime assertions. The standalone CPU fixture does not construct RuntimeContext or invoke its legacy effect service; the placement of that call outside both new host scopes is verified from the staged source. No production native files were modified.

Previous patch SHA256 (superseded): `a79786c883b8a67684f40692998a60d79c72ebc51aab35fa4c7a1cc6f6a66095`.
Corrected patch SHA256: `5b00861224eb96f648ef2f61323717a9da077e59cf8ae948669b651d73c83658`.

`native.patch`, `native/`, `native-manifest.json`, `native-compiles.json`, `native-verification.json`, and `probe-runtime.log` are re-frozen together. Apply the current `native.patch` only; the previous patch was not applied by the parent.
