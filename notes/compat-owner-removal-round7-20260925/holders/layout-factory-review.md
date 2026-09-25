# Layout factory review and repair

## Findings

The retired ResourceHolderService deduplicated layout holders by archive path. Routing MR::createLytTexMap through original ResourceHolderManager::createAndAddLayoutHolderRawData changed that behavior: every call added a full LayoutHolder and table set to the fixed 512-entry registry. StaffRollPicture::initReplaceTexture makes 20 calls on StaffRollTexture.arc, while PictureBookLayout makes one per page plus three cover/title calls. This creates avoidable holder/table growth and eventual registry exhaustion.

A second regression was type confusion in the shared original basename registry. A texture-first request published a LayoutHolder for an object texture archive. Subsequent createAndAdd/loadTexFromArc for that basename returned the same record's null ResourceHolder. The original factory never publishes layout holders: decomp/src/Game/Util/LayoutUtil.cpp uses loadTexFromArc, JUTTexture, then a new NW4R TexMap from its GXTexObj.

LayoutRuntime's actual-holder pointer plus retained native token preserves the previous ownership barrier. Native records are explicitly destroyed before remaining runtime fields and the token is released after all texture/animation state. Nw4rLayoutRecords' temporary resource accessor transfers native texture state to Material/AnimTransformBasic; it does not leave an accessor pointer in those consumers. No additional regression was identified in this bounded source review.

## Repair

The actual Game/Util/LayoutUtil.cpp now supplies the factory through the donor resource query/JUT conversion; LayoutUtilCompat no longer supplies it. Raw/stationed manager semantics remain unchanged. The factory creates each TexMap on the caller's allocation context, as the donor does, rather than allocating it in an auxiliary layout holder.

The canonical SDK restores JUTTexture::getTexObj and the donor TexMap GXTexObj constructor. TexMap constructors (including copies) register the existing JKR heap finalizer; its destructor unregisters. This releases native shared backing on explicit destruction and bulk heap reuse, including original raw-new factory objects. Host/stack maps are ignored by JKR finalizer registration. Copy assignment preserves the destination finalizer registration.

ResourceHolder retains its native BTI allocations individually and supplies their independent lifetime token. Factory copies retain encoded image bytes without pinning the original holder/archive, and without decoding or allocating another MEM1 image. LayoutHolder's unnecessary texture factory and owned texture vector are removed.

## Focused coverage and limits

OriginalResourceHolderTests adds a texture-first synthetic actual FileLoader mount, proves the resulting entry is a ResourceHolder, requests 520 textures (more than the entire registry capacity) while checking holder/image identity and unchanged MEM1 use, then verifies caller-heap allocation, image copying past holder/archive retirement, and final mapped allocation reclamation by bulk heap reuse. TPL checks remain attached to actual LayoutHolder bounded sources plus the existing SDK decoder; the donor game factory takes BTI/ResTIMG, so the former service-only TPL factory shortcut is removed. All prior TPL palette/LOD/GX checks remain.

`factory-before.json` records exact pre-edit hashes/status. `before-factory-fix/` includes already-dirty parent LayoutUtilCompat and holder/test sources; `factory-fix.patch` isolates this repair. New canonical SDK and Game utility owners were clean before this repair. No build or runtime claim is made here; the parent controls the integrated/focused build driver.

The first parent test run after restoring the donor path revealed an obsolete negative fixture: an absent archive used to raise in the removed service, while the original FileRipper correctly panics. That check now uses the explicit invalid-name guard and a missing texture within the actual mounted factory archive; no production missing-file behavior was changed. The test flushes progress so subsequent failures retain the completed assertions in its log. The source comparator accounts for that isolated fixture-contract change while preserving all SDK descriptor, palette, LOD, GX and mapped-backing checks.

### Original loading-worker fixture context

The next retry reached the new factory cohort and blocked in FunctionAsyncExecutor::waitForEnd (see the parent's non-reparenting `factory-wait-sample.txt`). This is not changed SDK thread identity: the donor manager's uncached createAndAddInner always waits for an async record, while original startOnMainThread runs inline without a record on main. Normal uncached resource construction must originate on the loading worker.

The fixture now runs only its texture-factory, manager-lifetime and retail model/layout load cohorts through the actual original FunctionAsyncExecutor worker. Its actual main callback services executor.update and releases guest CPU during short native waits. Worker assertions/errors are retained until original completion is joined. Raw/stationed duplicate creation remains on main, preserving those synchronous original entry points. Both context identities are asserted. No manager, SDK or production dispatch behavior changes. Exact pre-edit test and delta: `before-worker-fixture.cpp`, `worker-fixture-fix.patch`.

### Synchronous fixture allocation policy

The parent retry passed the 520-call factory/copy/finalization cohort, then failed entering raw layout creation. The diagnostic callback deliberately holds HostAllocationScope for test scaffolding; original CurrentHeapRestorer selects its target heap without changing this routing policy. Direct raw/stationed factory calls now have a narrow ClientAllocationScope so their holder allocations use the selected mounted heap, matching real Game callbacks. No JKR lock is carried across an async wait, and production code is unchanged. The fixture prints failed assertion messages before throwing, so a later archive-retirement guard during arena unwind cannot obscure the primary assertion. Exact snapshot/delta: `before-layout-allocation-fixture.cpp`, `layout-allocation-fixture-fix.patch`.

### Shared original worker helper

After OriginalResourceHolderTests passed completely, the adopted NameObjFactoryPlacement real-process test exposed the same uncached-main-thread preload precondition for InvisibleWall10x10.arc. The proven worker helper is now shared in OriginalStageResourceProcessFixture.hpp. Resource holder tests use the same helper, and only the NameObj wall-resource preload runs through it; placement construction and cached checks remain on main. The helper asserts both real SDK contexts, services the actual original executor queue and joins its original completion record. It changes no production dispatch. `shared-worker-before.json`, `before-shared-worker-fixture/`, and `shared-worker-fixture-fix.patch` preserve both preexisting dirty test/header files, including the parent's AreaObj.hpp addition.
