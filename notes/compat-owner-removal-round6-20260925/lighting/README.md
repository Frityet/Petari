# Original lighting owner restoration

Baseline: root eb0712c1488fdc244f7daa0e8da42ea89455d7df; donor decomp 1a126cb5d. Complete original implementations now own the lighting catalog, zone lookup, actor interpolation, light registration and GX submission. Eight obsolete provider/cache files are deleted. No Aurora or RuntimeContext production edits were needed.

The six complete restored TUs are Map/LightDirector, Map/LightFunction, Map/LightDataHolder, Map/LightZoneDataHolder, Util/LightUtil and LiveActor/ActorLightCtrl. All 61 donor definitions remain present. ActorLightCtrl calls the real DrawBuffer::resetLightSort instead of the former no-op. LightFunction uses original camera-space conversion before blending, original unclamped interpolation, and actual GX commands in every host context.

The previous data cache and binder are removed: render/light/LightData and scene/StageLightSceneBinding, along with OriginalSceneSupport's light-only hook/member and SceneInitializationCompat's extra hook call. Original GameScene's existing initLightData call now initializes the real ResourceHolder/JMapInfo-based owners at the original point before actor placement. The literal catalog default and the stage default remain separate original behaviors.

## Native boundaries

- Japanese names use the existing CP932 literal helper. Complete native types are included explicitly where the donor relied on its umbrella headers.
- LightDirector keeps exception-safe allocation of its three owned children. Its destructor retires their actual owned records. LightDataHolder, LightZoneInfo and LightZoneDataHolder destructors release their arrays. Original JMapInfo parser allocations remain scene-heap owned: extracted string pointers must outlive the light rows, and JMapInfo already registers its native disposer.
- Player-light and area-manager registration maintain a borrowed-owner backlink in the actual ActorLightCtrl/LightAreaHolder. Destroying either borrower or director clears the corresponding relationship without looking up a retired scene, allocating a replacement, or retaining a global registry. The two ActorRuntimeRegistry manual unregister calls and compat diagnostics are removed.
- LightInfo/LightInfoCoin/ActorLightInfo and the zone holders regain their donor declarations. Coin light data now uses the original composed base record and GXColor rather than a native inheritance/byte-field substitute.
- LightPointCtrl.cpp/hpp are byte-for-byte unchanged: generation-qualified borrowed actors, owned point records, PPC NaN/conversion rules and duration-zero behavior survive. LightFunction's point submission maps the current native record field names and casts its existing u32 attenuation kind to the SDK enum. This is the only point-record integration adaptation.
- OriginalModelLightAccess remains pending the complete ModelUtil/LiveActorUtil ownership closure. No methods were relocated into a partial replacement file.

## Test changes and limits

PointLightRuntimeTests retains separate `--original-owner-only` and `--original-gx-only` modes. Its default now runs inside OriginalStageResourceProcessFixture, covering the actual root and child-zone light catalog, default-name behavior, real player registration, world/view-space interpolation and extrapolation, native stale/ABA pointer handling, brightness/zero-duration rules, point-light cosine endpoints/midpoints and the existing exact GX command checks. The fixture restores temporarily modified camera matrices and player position before the next game frame.

The former successful standalone RuntimeContext/SceneLightService path and its invented no-player fallback expectations are retired. AreaObjRealOrAbsentTests retains area geometry/priority/ZoneLightID checks; its obsolete separate cache fixture and resource-less actor-light integration were removed. Real catalog/player coverage now lives in the process test.

OriginalLightFixture.hpp supplies the original LightDirector-before-AreaObjContainer initialization order to existing standalone area consumers. It records/discards the actual white-light GX setup commands so data-only fixtures do not need device submission. Six other test files have only this prerequisite/include adjustment. Five were initially dirty. Their exact before snapshots, working-only patches and minimally adjusted HEAD-based blobs are under staged-test-adaptations; root must stage those HEAD-based blobs, not unrelated working rewrites.

## Validation and build integration

`python3 notes/compat-owner-removal-round6-20260925/lighting/validate-source.py` passes 141 source checks, including complete donor coverage, unchanged original method bodies outside five explicitly listed native boundaries, unchanged point controller implementation, and no retired source/test references. These are source checks, not runtime results.

`build-wiring.json` lists the three excluded Game files to enable, removed explicit render source/header registrations, and point-light target app/main dependencies. No new compile flags are needed. The parent task owns builds, focused tests, fresh-save execution, formatting, index changes and publication. Supporting before hashes, final owned paths and exact patches are recorded separately.
