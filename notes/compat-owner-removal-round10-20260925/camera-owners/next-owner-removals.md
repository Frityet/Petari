# Next canonical owner removals

Read-only review of the current round10 tree. No sources, tests, build wiring, or Git index changed. Recommend the NameObj execution closure first; layout is a larger separate ownership change.

## 1. NameObjExecuteHolder: smallest useful complete owner

`src/compat/OriginalNameObjExecuteHolder.cpp` currently provides the whole holder/info implementation and MR wrappers. The canonical port `.cpp` is absent; the complete donor is `decomp/src/Game/NameObj/NameObjExecuteHolder.cpp` (422 lines). Its canonical header is already present. The donor original-process lookup has all required owners: `GameSystemSceneController::getNameObjListExecutor()` returns the actual scene executor, and the scene creates `SceneObj_NameObjExecuteHolder`.

Proposed batch:

- Add the complete donor at `src/Game/NameObj/NameObjExecuteHolder.cpp`; remove `src/compat/OriginalNameObjExecuteHolder.cpp`.
- Restore original SceneObj/GameSystem lookup for actual game operations. Use a CP932 literal wrapper for the original constructor name rather than the provider's dynamic string helper.
- Add native holder destruction in its canonical header/source: `mExecuteArray` is allocated with `new[]` and has no current owner destructor. Keep destruction after scheduler retirement.
- Preserve the native registration boundary explicitly until the scheduler itself is consolidated. Current `MR::registerNameObjToExecuteHolder` calls `SceneScheduler::register_name_obj`; replacing it with the donor direct call immediately loses required bookkeeping.

`src/runtime/SceneScheduler.cpp::register_execution_entry/register_name_obj` retains each original ModelManager owner, validates draw registration, grows original category arrays for late registration, initializes late connections, registers native draw buffers, and rolls back failed registration. These are actual host lifetime/render constraints, not duplicate game execution logic. The canonical holder should retain only the necessary native boundary (or the owning scheduler should expose a narrow registration operation); do not copy the entire scheduler into Game. Preserve existing capacity checks. `src/scene/SceneExecutionBinding.cpp::prepare_retirement` clears scheduler entries before the binding/actual scene owners retire.

Wiring: Game's source glob includes the new canonical file; remove the old provider through the existing source glob. Existing `OriginalSceneExecutionOwnerTests.cpp` exercises the real holder; no new fixture is necessary for this removal.

## 2. Related SceneFunction closure: two more providers, with scoped host boundaries

`src/Game/Scene/SceneFunction.cpp` is already complete and matches the donor (138 lines), but `src/Game/xmake.lua` excludes it. Restore that compiled owner and delete `SceneInitializationCompat.cpp` plus `SceneMovementCompat.cpp` only after accounting for these native boundaries:

- `startActorPlacement`: current coverage reporting before original placement is optional diagnostics.
- `initEffectSystem`: current `initialize_original_scene_effects` establishes actual effect resource/lifetime ownership; donor creates the original EffectSystem and enters the actual particle holder. Keep the necessary native ownership in its owner.
- `allocateDrawBufferActorList`: `SceneExecutionBinding::complete_initialization` calls scheduler draw allocation, then actual holder `initConnectting`, and records initialization state.
- Movement/draw entry points currently supply Game callback allocation, `SceneJ3dScope`, and renderer draw-registration scopes through the scheduler. Retain those host boundaries around original executor operations. The no-binding fallback to `RuntimeContext::scheduler()` is an obsolete alternate-process path, not a donor requirement.

`SceneConnectionCompat.cpp` is different: it defines the native permanent-retirement API `MR::disconnectToScene`, which has no donor declaration. The scheduler operation removes original pending execution, native draw-buffer/model borrows, and registration records. Keep it pending until actual native ownership callers can invoke their retirement owner directly; then remove the invented Game declaration and provider. Do not disguise it as a restored donor function.

## 3. OriginalLayoutLocale: belongs to a larger LayoutManager restoration

`src/compat/OriginalLayoutLocale.cpp` provides `LayoutManager::removeUnnecessaryPanes`, not LayoutUtil. The current compiled canonical `src/Game/Screen/LayoutManager.cpp` contains only ten methods (121 lines); the donor contains the full owner (733 lines), including the locale collection/removal algorithms. `src/layout/Nw4rLayoutRecords.cpp` currently invokes the locale provider while constructing an alternate layout record graph.

The complete closure is:

- Restore `Game/Screen/LayoutManager.cpp/.hpp` from the donor and remove `OriginalLayoutLocale.cpp`.
- Remove all LayoutManager definitions from mixed `src/layout/LayoutManagerCompat.cpp`; retain its unrelated LayoutActor/LayoutPaneCtrl definitions until those owners are restored. Deleting this entire provider additionally requires restoring/enabling both canonical LayoutActor and LayoutPaneCtrl sources, currently excluded in Game/xmake.
- Migrate `layout/Nw4rLayoutRecords` and `LayoutRuntime` to the actual manager-owned graph/resource lifetime. Current native manager header has opaque/relabelled fields (`_64`, `_70`, `_74`, `_78`, and `void*` group links) where donor has full typed pointers/counts. Restore those types and adjust native consumers coherently, especially the full-width indirect texture pointer.

There is no missing SDK Layout::Build implementation: `src/nw4r/lyt/lyt_layout.cpp` already runs the original pane/group construction over `NativeLayoutResource` endian decoding and retained storage; CreateAnimTransform is also implemented. The full manager would replace unavailable/no-op alternate implementations of initArc, initDrawInfo, initGroupCtrlList, replaceIndDummyTexture, and pointing/animation operations.

Required native details: retain the actual LayoutHolder token; destroy layout/animation/pane/group controllers in dependency order; handle the indirect TPL texture's archive range with full-width bounded pointers rather than donor `(void*)-1` pointer comparisons. Locale pruning unlinks panes but must not immediately delete them: group and animation links can still reference detached panes. Current `Nw4rLayoutRecords::State` unbinds active and detached animations, destroys layout/groups, then clears intrusive child lists and frees panes. Preserve that lifetime meaning in the actual owner.

Preserve donor locale selection exactly: when the selected language exists in a language-suffixed group, remove every other sibling, including neutral siblings; otherwise remove recognized-language children only. Recurse only when no such group was detected. Moving only the 104-line locale fragment would remove a filename but leave the alternate owner in place; defer that shortcut.

Suggested order: NameObjExecuteHolder, then the scoped SceneFunction pair, then the full LayoutManager/LayoutActor/LayoutPaneCtrl graph in its own batch. No new tests or broad fixture work proposed; root can use the existing actual-process smoke for each integrated batch.
