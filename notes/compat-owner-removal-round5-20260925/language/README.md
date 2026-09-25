# Original process language owner

Baseline: `2fef93f2c570db367556b9f556770bb16a0b8289`. Reference: decomp `1a126cb5da311fedff662f53fb31c5aeaf851408`.

## Changes

- Remove `src/compat/OriginalLanguage.cpp` and `LanguageOwnership.hpp`. Enable the already complete, byte-identical donor `src/Game/System/Language.cpp` through root's build wiring. `MR::getLanguage` now reads only the original `GameSystem -> GameSystemObjHolder::mLanguage` field.
- Remove the LanguageOwnership include, owned member, and construction in `runtime/MessageHolderOwnership.cpp`. The constructor's `language_prefix` remains explicit archive-path metadata; it no longer changes global Game language.
- No extra region or encoding adaptation was needed. The original SC language accessor already sanitizes inputs to 0..9, and the current donor is the Korean build with the original row-4 regional mapping. No alternate unsupported region or fallback language was invented.
- `OriginalSystemProductInfo.cpp` was identified as the Wii SDK `scapi_prdinfo.c` implementation (`__SCF1`, `SCGetProductArea`, `SCGetProductGameRegion`), not Game language logic. Root owns its SDK consolidation; this lane did not edit it.

No new decompilation, SDK changes, FileUtil changes, OriginalGameApplication changes, RuntimeContext changes, build configuration edits, builds, staging, or commits were performed in this lane. Snapshot/manifest files preserve every owned baseline.

## Fixture migrations

- Replace the obsolete `LanguageOwnershipTests.cpp` with `OriginalLanguageTests.cpp`. It checks the actual initialized owner's SC-decided language, then exercises all twelve original language IDs/prefixes and region-prefix extraction through the same original object field. A local guard restores the real field before the process continues. No alternate language publisher is constructed.
- `LayoutRealOrAbsentTests.cpp` now runs inside the real process. Its metadata/absent-resource assertions remain; an uninitialized actor's effect test now correctly requires an initialized LayoutManager instead of a missing RuntimeContext. The invalid request to initialize a deliberately missing Game resource was removed from setup.
- `OriginalJpaManagerTests.cpp` now obtains the real process's `ParticleResourceHolder`, uses a child Game heap for its independent emitter pool, and retains every resource-name lookup, all 3,327 emitter IDs, callback lists, optional sixteen-frame CPU sampling, pool exhaustion and reuse checks. Child-heap retirement is checked separately from weak particle backing, which must expire only after the actual process exits. The old redundant ParticleResourceHolder/manual DVD/ArchiveMountService bootstrap is removed.
- `OriginalLayoutGroupTests.cpp` obtains locale selection from the actual process. Synthetic typed pane/group/material graphs, sixteen repeated localized graph lifetimes, detached group borrowers, animated pane sizes, text tags, and explicit retail-layout mode remain. Heap-boundary checks use a child of the real scene allocation domain. FlyMeter now uses the initialized real GameScene and the actual active Aurora frame, preserving the three life-ratio draw and allocation checks without creating a second window, renderer, RuntimeContext, or SceneExecutionFixture.

All four tests use `OriginalStageResourceProcessFixture`, a fresh temporary save directory, and `SMGPC_REAL_DISC`. The original locale fixtures require the supplied Korean retail disc; they explicitly verify its actual `KrKorean` selection. Pure unbound resource viewers remain unlocalized, as before.

## Root wiring

- Remove `remove_files("System/Language.cpp")` from Game's exclusion list.
- Rename the old language-ownership target/source/test registration to original-language / `OriginalLanguageTests.cpp`; do not retain an old target alias.
- Add `smg-pc-app` and `aurora-main` dependencies to original-language, layout-real-or-absent, original-jpa-manager, and original-layout-group.

## Remaining standalone message closure

`MessageHolderOwnership`'s standalone constructor already cannot bootstrap with the restored strict original FileLoader and rejects coexistence with an actual GameSystem. Removing its language publication does not turn it into another process owner. A coherent later removal should delete that standalone class and RuntimeContext's field/bootstrap, migrate `OriginalMessageHolderTests`, and consolidate the still-used actual process destruction, scene-message binding, and pointer-to-message debug lookup in their actual owners. Root's resource lane was notified. No substitute GameSystem or relocated LanguageOwnership was added.

## Validation status

The canonical Language donor comparison is exact. No source/test reference to the removed language publisher remains except the root-owned test wiring pending update. Targeted diff whitespace checks passed. Integrated compilation/runtime results are pending root validation; source readiness does not establish test or gameplay success.

## Root runtime results and FlyMeter fixture correction

Root reported passing original-language, layout-real-or-absent, and original-jpa-manager. The first original-layout-group run failed its FlyMeter setup assertion. Its `LayoutRuntime::getArchivePath()` assertion still tested standalone preview metadata: the actual mounted-archive constructor retains its `LayoutArchiveOwner` and does not populate `mArchivePath` (`LayoutRuntime.cpp:296`, getter at 379). This is a fixture migration error, not a missing archive.

The corrected fixture resolves FlyMeter with the original language/aspect helper, compares the actual `LayoutHolder::mArchive` with `MR::receiveArchive`, verifies authored FlyMeter.brlyt / Count.brlan and the Count pane/controller, and checks exact stopped Count frames for all three life ratios before drawing. The former path assertion is removed. No production changes were made. Pre-correction fixture is captured in `before-flymeter-fixture-fix.cpp`. Rebuild and runtime validation remain root-owned and pending.
