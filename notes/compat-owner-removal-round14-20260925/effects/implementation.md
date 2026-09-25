# Round14 actual effect ownership

Baseline: `e7b214efc`. All 26 owned files were snapshotted before edits; see `owned-manifest.json` and `before/`. `effects-only.patch` excludes the root's subsequent J3DSys change in shared LiveActor.cpp. The gravity lane owns the coordinated two-line effect cleanup in ActorRuntimeRegistry.cpp; the root owns SceneObjHolderCompat.cpp, SceneObjHolderRuntime.hpp, and build wiring.

## Change

Deleted EffectSystemOwnership.cpp/.hpp, including its global actor/layout keeper maps and standalone alternate particle/heap fallback. EffectSystem now retains its actual caller's Game allocation domain, owns its existing draw/calc executors, group holder, emitter holder and manager, and tracks only live keeper borrowers. Native retirement deletes those keeper graphs before force-deleting remaining emitters. The original actor pointers are cleared when a keeper retires, so later actor destruction is safe.

Restored the donor LiveActor::initEffectKeeper and LayoutActor::initEffectKeeper allocation/initialization bodies. Actual EffectKeeper and PaneEffectKeeper destructors retire every emitter borrowing their SingleEmitter records, including older one-shot replacements tracked by the existing full-width mLastNonzeroUserWork token. The actual MultiEmitter, SyncBckEffectInfo, AutoEffectGroup and AutoEffectGroupHolder destructors release their own children. MultiEmitter child links and AutoEffectInfo references remain borrowed; they are not recursively freed.

ParticleDrawExecutor and ParticleCalcExecutor own their existing eleven original NameObj adaptors. Each adaptor is claimed immediately through the existing registration ownership boundary, so scene construction records it without acquiring duplicate delete ownership. Constructor rollback and executor destruction delete adaptors in reverse order, retiring their registered callbacks and functors before the executor storage. This removes the need to delete an entire scheduler registration suffix as an effect-specific workaround.

EffectSystem entry obtains a borrow token from the actual ParticleResourceHolder. That token is a lifetime assertion, not ownership of freed heap bytes: the actual process holder rejects premature destruction while any effect system still borrows its resources. Normal process ordering already destroys scenes before that holder. The separate EffectSystem constructed by ScenarioSelectScene is now explicitly deleted by its actual owning scene destructor; its previous empty destructor would otherwise retain that borrow through process shutdown.

Original emitter creation, calculation, drawing, effect registration, and group lookup algorithms remain unchanged. mEmitterHolder is now initialized to null so partial entry failures have a defined cleanup state. Native resource entry rejects duplicate/retired entry and zero pools. JPA raw arrays continue to belong to the retained original heap, as before; this change does not add an alternate allocator or particle catalog.

## Shared integration supplied to root

- Scene factory case uses the original `new EffectSystem(name, true)`.
- Scene initialization enters its actual `_game_allocation_domain` and calls `entry(MR::getParticleResourceHolder(), particles, emitters)`.
- Scene retirement calls the actual EffectSystem slot's `retireNativeResources()` before child destruction.
- Remove EffectSystemOwnership member/getter, scheduler marker/suffix cleanup, byte-budget overload, and helper construction/reset.
- ActorRuntimeRegistry removes the old include/release helper and directly deletes/clears the actor's actual EffectKeeper; gravity lane applied this coordinated edit.

## Obsolete test removal and validation limit

Deleted the existing OriginalEffectOwnershipTests.cpp and requested removal of `smg-pc-original-effect-ownership-tests`. Its bootstrap depends on the removed standalone RuntimeContext particle publication, injected scene binding, an invented separate effect allocation domain, and byte-budget failure expectations. Retaining that setup would preserve the substitute ownership path; migrating its mixed rendering/resource/rollback assertions would require a fixture rewrite. No replacement test framework or new test cases were added, following the requested faster scope.

No builds, test runs, staging, or commits were performed by this lane. Read-only `git diff --check` passed, and a source/test search found no remaining references to the deleted EffectSystemOwnership API after shared integration. The root runs the integrated app build and short smoke; no runtime success is claimed here.
