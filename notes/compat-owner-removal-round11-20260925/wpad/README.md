# Round 11 WPad and StarPointer ownership

Removed eight compat files: WPadOwnership.cpp/.hpp, StarPointerDepthOwnership.cpp/.hpp, OriginalStarPointerDirector.cpp, OriginalStarPointerDepth.cpp, OriginalStarPointerOwnerQueries.cpp, and StarPointerServiceCompat.cpp.

## Actual owners

- WPadHolder's internal lookup is the exact donor GameSystem -> GameSystemObjHolder -> WPadHolder expression. Existing native WPadHolder callback cancellation and child destructors remain unchanged.
- MemoryUtil contains the donor allocFromWPadHeap/freeFromWPadHeap pair using the actual HeapMemoryWatcher::mWPadHeap. Neither callback consults an alternate process publication.
- StarPointerDirector.cpp restores the complete current donor source, including transform, peek-Z and StarPointerFunction definitions. The existing native defined-size projection initialization and null-array arithmetic guard are retained; the guidance name uses CP932 bytes. Its new destructor is the existing process cleanup: quiesce GX/DrawSync, unregister its peek callback, retire guidance spines/layout children and arrays, then controllers/transform/peek storage. Child layout destruction uses the actual LayoutActor destructor. No generic service was renamed.
- StarPointerUtil.cpp is the complete current donor source. All original process query, mode, guidance, target, pane-hit, pointer-depth and rumble behavior now comes from that owner. The custom pane-hit shortcut and fallback runtime pointer publication disappear. The donor already uses u64 actor/layout identities and the canonical LiveActor target initializer, so no added pointer-width adapter is required.
- RuntimeContext no longer constructs or updates its duplicate pointer/WPad process. OriginalGameApplication destroys its actual director and asks actual LayoutActor objects to release native resources during its existing heap-retirement prepass (coordinated with the layout owner restoration).

## Existing tests

FrameButtonState, OriginalWPadAcceleration, and OriginalWPadPause now use the existing actual-process fixture and actual GameSystem WPad objects. Their useful button/debug-script, acceleration, and pause/Home assertions remain. Synthetic nested/failed/repeated alternate-owner generations are retired. The old ownership test retains its SDK callback cancellation check; the gesture test retains its SDK physical gesture contract; pointer-input retains its independent retained-message-storage check. OriginalStarPointerOwner reads its actual LayoutManager::mLayoutName; its resource and GPU assertions remain. SphereSelector drops only the removed alternate-pointer-publication absence contract.

No new tests or fixture framework were added. No builds or test runs were performed. Full before snapshots (including preexisting dirty App/RuntimeContext/test work), exact per-path hashes, and this batch's scoped patch are included. Root owns build-graph integration and publication.
