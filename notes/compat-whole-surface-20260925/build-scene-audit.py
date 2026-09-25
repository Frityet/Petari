#!/usr/bin/env python3
"""Record ownership review separately from completed migration/behavior validation."""
import hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
# destination, disposition, specific reason / remaining removal work
spec={
'AreaObjRuntimeCompat':('Game/AreaObj/AreaObjContainer.cpp; MercatorTransformCube.cpp','restore-original-owner','Replace duplicate manager registry with original 67-row table. Preserve unavailable Mercator method at actual Game owner until recovered.'),
'DemoCompat':('Game/Demo/DemoDirector.cpp; tests','delete-facade','Four wrappers only forward membership, action counts and actor retirement to DemoSceneRuntime. Replace consumers with original owner queries.'),
'DemoDirectorOwnership':('Game/Demo/DemoDirector.cpp; DemoCastGroup.cpp; Demo*Keeper.cpp','narrow-game-adaptation','Sidecar owns group children and clears borrowed cast/request pointers; retain retirement semantics in actual destructors before removing external owner.'),
'DemoSceneRuntime':('Game/Demo/DemoDirector.cpp; DemoExecutor.cpp; tests','delete-facade','Parallel archive placement/registration facade over original DemoFunction. Audit production vs fixture consumers and remove manual placement/update path.'),
'DemoSheetRuntime':('Game/Demo/DemoTimeKeeper.cpp; Demo*Keeper.cpp; tests','delete-facade','Duplicate parsed sheets, timer, pause, subpart and final-boundary state machine. Original keepers must be sole behavior source.'),
'DemoStartRequestOwner':('Game/Demo/DemoStartRequestHolder.cpp','narrow-game-adaptation','Heap wrapper and deleter for proxy/start-info allocations; canonical holder destructor can own these.'),
'DemoUtilCompat':('Game/Player/MarioAccess.cpp; tests','delete-facade','release_puppetable_demo_control ignores boolean and calls endRemoteDemo(nullptr); remove wrapper after direct original call migration.'),
'GameDataFunctionCompat':('Game/System/GameDataFunction.cpp; tests','restore-original-owner','Fragmented original GameDataFunction plus thread-local test overrides and missing-operation errors. Restore original user-file/sequence owner and migrate alternate session fixtures.'),
'GameDataOwnership':('Game/System/GameDataHolder.cpp; GameEventFlagTable.cpp','narrow-game-adaptation','Native teardown manually walks save children and process event table. Place destruction and static ownership at real classes; preserve scene arena retirement.'),
'GameDataSession':('Game/System/SaveDataHandleSequence.cpp; UserFile.cpp; tests','delete-facade','Alternative selected-file bootstrap synthesizes config and snapshots and publishes thread-local overrides. Original process owns user files already.'),
'GameRuntimeCompat':('Game/Util/LiveActorUtil.cpp','restore-original-owner','Original clipping sphere/far/group methods; single native group lifetime capture should attach at ClippingDirector owner.'),
'GroupCheckManagerCompat':('Game/Player/GroupChecker.cpp','restore-original-owner','unordered string membership and two-group sidecar replace original group manager; restore original group records and retain only native width/lifetime adaptations.'),
'HashSortTableCompat':('Game/Util/HashUtil.cpp; MathUtil.cpp','restore-original-owner','Hash table methods plus unrelated unsigned sorting overload. Native Value width is required; split methods by canonical owner.'),
'InformationMessageCompat':('Game/Screen/InformationMessage.cpp; GameSceneLayoutHolder.cpp','narrow-game-adaptation','Alternative global binding captures constructor children; canonical layout holder should own message and rollback/teardown.'),
'MessageUtilCompat':('Game/Util/MessageUtil.cpp; Game/System/MessageData.cpp','narrow-game-adaptation','Direct UTF16 access is necessary when native wchar_t differs. Keep resource-backed conversion at actual message owner and delete extra compat header.'),
'MtxCompat':('Game/Util/MtxUtil.cpp','restore-original-owner','Extracted matrix/orientation methods and native implementation fragments belong to original MtxUtil. Preserve unfused/fused behavior explicitly.'),
'NameObjExecuteCompat':('Game/NameObj/NameObjFinder.cpp','restore-original-owner','Single find function through alternative registry; original process NameObjHolder lookup can replace it.'),
'NameObjLifetimeCompat':('Game/NameObj/NameObj.cpp','narrow-game-adaptation','Entire duplicated NameObj TU. Only constructor registration/rollback and destructor retirement differ; integrate these targeted changes into original owner.'),
'Nw4rDiagnostics':('nw4r/db/db_assert.cpp','narrow-library-adaptation','Host exception/reporting boundary for Panic; canonical library owner should provide native implementation, without Game policy.'),
'Nw4rLayoutRecordsCompat':('nw4r/lyt/lyt_pane.cpp','restore-original-owner','Pane constructors, traversal, animation, matrix and resource access implementations belong to nw4r Pane. Verify native decoded records on import.'),
'Nw4rLinkListCompat':('nw4r/ut/ut_LinkList.cpp','restore-original-owner','Complete original intrusive list implementation; canonical source can compile directly.'),
'OriginalAlphaClear':('Game/Util/DrawUtil.cpp','restore-original-owner','Original alpha-buffer GX drawing methods; same owner as clear-Z fragment, not a PC policy service.'),
'OriginalClearZBuffer':('Game/Util/DrawUtil.cpp','restore-original-owner','Original clear-Z texture setup and GX pass; consolidate alongside alpha clear without losing static texture state.'),
'OriginalFurAccess':('Game/Util/LiveActorUtil.cpp','restore-original-owner','Three original Fur construction entry points; restore to utility owner.'),
'OriginalGCaptureQueries':('Game/MapObj/GCapture.cpp','restore-original-owner','Original presence/state query for GCapture. Do not invent capture state when restoring actor owner.'),
'OriginalGameDiagnostics':('Game/Scene/SceneObjHolder.cpp; Game/Util/PlayerUtil.cpp','narrow-game-adaptation','Debug dependency checks should be local debug assertions or proper owner checks, not a standalone compat API.'),
'OriginalGameSystemAccess':('Game/Util/SystemUtil.cpp; SceneUtil.cpp','restore-original-owner','Original process-holder/message/random-seed/audio/layout allocator accessors; split by canonical donor owner.'),
'OriginalJointAccess':('Game/Util/JointUtil.cpp','restore-original-owner','Original joint matrix/name queries; integrate native index/pointer boundary directly into utility owner.'),
'OriginalJutCapture':('JSystem/JUtility/JUTTexture.cpp','restore-original-owner','JUTTexture GX copy-texture method; consolidate with other texture providers.'),
'OriginalLayoutLocale':('Game/Screen/LayoutManager.cpp','restore-original-owner','Original language-pane filtering and local BitFlag; resource decoding/native pointer ownership remain in layout owner.'),
'OriginalMapQueries':('Game/Util/MapUtil.cpp','restore-original-owner','Original collision director query/sort buffer algorithms; no independent host map representation needed.'),
'OriginalMirrorReflectionUtil':('Game/Util/LiveActorUtil.cpp','restore-original-owner','Original mirror/submaterial setup. Consolidate with model/texture utility methods at donor owner.'),
'OriginalMtxGeometry':('Game/Util/MtxUtil.cpp; MathUtil.cpp','restore-original-owner','Mixed matrix and math methods; split at actual donor method owners while preserving rounding.'),
'OriginalNameObjExecuteHolder':('Game/NameObj/NameObjExecuteHolder.cpp','narrow-game-adaptation','Full original executor methods with native arena capture, registration guards and teardown. Move necessary changes directly into owner and remove provider fragment.'),
'OriginalPointerVectorQueries':('Game/Util/MathUtil.cpp','restore-original-owner','Four original vector/scalar normalization helpers; include in full MathUtil restoration.'),
'OriginalSceneCounterQueries':('Game/System/GameDataFunction.cpp','restore-original-owner','Counter accessors use original data but retain alternate StageSession fallback. Remove fallback after fixtures use process temporary galaxy data.'),
'OriginalSceneDrawUtil':('Game/Scene/SceneFunction.cpp','restore-original-owner','Spin-driver opaque-pass and spider-thread bloom queries use original scene objects. Include with full SceneFunction restoration.'),
'OriginalScreenConfig':('Game/Util/SystemUtil.cpp','restore-original-owner','Original isScreen16Per9 forwarding method; no independent compat behavior.'),
'OriginalStarPointerDepth':('Game/Screen/StarPointerDirector.cpp; Game/Util/StarPointerUtil.cpp; ScreenUtil.cpp','restore-original-owner','Mixed depth owner/GX callback and utility fragments. Restore canonical owner and retain native draw-sync token lifetime.'),
'OriginalStarPointerDirector':('Game/Screen/StarPointerDirector.cpp','narrow-game-adaptation','Original director methods plus CP932 strings and alternate bootstrap fallback. Canonical process holder must own controller/layout state.'),
'OriginalStarPointerOwnerQueries':('Game/Util/StarPointerUtil.cpp','restore-original-owner','Original pointer modes/reaction/command/owner queries; consolidate fragmented providers and remove hidden alternate ownership fallbacks.'),
'OriginalSystemConfigAccessors':('aurora/lib/sc','general-platform','Retail SC field semantics depend only on Wii system configuration. Move to Aurora SC beside its persistence implementation, ensure one provider.'),
'OriginalSystemProductInfo':('aurora/lib/sc','general-platform','Retail product-area/region parsing is Wii SDK behavior. Keep byte identities and configured product data at SC provider.'),
'OriginalTripodBossQuery':('Game/Boss/TripodBossAccesser.cpp; TripodBoss.cpp','restore-original-owner','Mixed accessor/boss joint query and bone-id conversion table; split by original source owner, preserve real boss state.'),
'OriginalVectorOrientation':('Game/Util/MathUtil.cpp','restore-original-owner','Four original vector orientation/angle methods; restore full math owner.'),
'OriginalWPadHolder':('Game/System/WPadHolder.cpp','restore-original-owner','Original holder methods with native WPAD callback dispatch and heap/lifetime boundaries; consolidate with actual input owner.'),
'OriginalWPadSpeaker':('Game/System/WPadSpeaker.cpp','restore-original-owner','Original speaker state and callbacks; keep generic Wii speaker service in Aurora.'),
'PPCArchCompat':('aurora/lib/base','general-platform','PPCSync memory-ordering boundary implemented with atomic thread fence. No Game-specific dependency; belongs Wii CPU API provider.'),
'PlanetMapRuntimeCompat':('Game/Util/LiveActorUtil.cpp; Game/Map/OceanHomeMapCtrl.cpp','narrow-game-adaptation','Submodel creation/LOD native ownership plus explicit two-name OceanHome unavailable check. Remove name policy via actual OceanHome owner implementation; preserve original submodel resources.'),
'RumbleCompat':('Game/System/WPadRumble.cpp; aurora WPAD service','delete-facade','Single RumbleActuator adapter forwards WPAD calls; eliminate alternate facade attachment where original WPadRumble already drives WPAD.'),
'SaveChunkEncoding':('Game/System/BinaryDataChunkHolder.cpp; concrete save chunks','narrow-game-adaptation','Chunk signature aware endian conversion, path/galaxy validation and PLAY prefix semantics. Required host width/endian fixes belong in save serialization owners, not Aurora Game schema.'),
'ScenarioDataParserAccess':('Game/System/ScenarioDataParser.cpp','restore-original-owner','Process parser lookup plus alternate ScenarioCatalog fallback. Consolidate at original parser function and migrate isolated fixtures.'),
'SceneConnectionCompat':('Game/Scene/SceneFunction.cpp','narrow-game-adaptation','Native retirement disconnect delegates scheduler and guards missing active owner; integrate with original scene connection functions.'),
'SceneInitializationCompat':('Game/Scene/SceneFunction.cpp','narrow-game-adaptation','Original initialization delegates plus native effects/lights/execution binding. Keep boundary hooks in owning scene methods and delete duplicated TU.'),
'SceneLifetimeCompat':('Game/Scene/Scene.cpp','narrow-game-adaptation','Only native lifetime binding/rollback changes are necessary; apply to original Scene source.'),
'SceneMovementCompat':('Game/Scene/SceneFunction.cpp','narrow-game-adaptation','CategoryList dispatch forwards native scheduler; consolidate in canonical methods and remove RuntimeContext fallback where real scene executor available.'),
'SceneMovementOwnerLifetime':('Game/Scene/SceneNameObjMovementController.cpp','narrow-game-adaptation','Eight-line native destructor for stop-state control belongs in actual class owner.'),
'SceneNameObjUtilCompat':('Game/Util/SystemUtil.cpp','restore-original-owner','Four original holder iteration/suspend/sync methods; alternate registry fallback can disappear with original process fixtures.'),
'SceneObjHolderCompat':('Game/Scene/SceneObjHolder.cpp; scene/SceneObjHolderRuntime.cpp','narrow-game-adaptation','Mixed original factory and native transactional adoption. Restore factory to Game; remove per-system shadow ownership as real destructors take over, retain general transaction only if necessary.'),
'SceneSystemUtilCompat':('Game/Util/SystemUtil.cpp; scene/PlacementZoneScope.cpp','narrow-game-adaptation','Unrelated PAL query and nested placement-zone scope. Split PAL original method and real placement-scope implementation by owner.'),
'SimpleLayoutCompat':('Game/Screen/SimpleLayout.cpp','restore-original-owner','Full original source becomes compilable with named -1 execution enums; use donor directly.'),
'SphereSelectorRuntimeCompat':('none','delete-unused','Unused internal getter and an empty namespace; no public provider to retain.'),
'StageScenarioMetadataResolver':('Game/System/ScenarioDataParser.cpp; GameAudio/AudStageBgmWrap.cpp; tests','delete-facade','Parallel scenario archive parsing, comet classification and audio bootstrap; actual process scenario/audio owners should be sole source.'),
'StageSessionGameCompat':('Game/Util/SystemUtil.cpp','restore-original-owner','Restart ID wrappers fallback to parallel StageSessionState. Original temporary galaxy data can supply direct canonical methods.'),
'StageSessionState':('Game/System/GameDataTemporaryInGalaxy.cpp; tests','delete-facade','Shadow scene/stage/scenario metadata and temporary-data bootstrap should be removed after standalone fixtures use actual process owners.'),
'StarPointerDepthOwnership':('Game/Screen/StarPointerDirector.cpp; Game/System/GameSystemObjHolder.cpp','narrow-game-adaptation','Alternative input/pointer arena/bootstrap and retirement sidecar. Use process-owned original objects and targeted destructors; retain draw-sync callback cleanup.'),
'StarPointerServiceCompat':('Game/Util/StarPointerUtil.cpp','narrow-game-adaptation','Pane hit-test facade ignores parameters and target construction fragments. Restore original full pointer algorithms, keep only native pointer identity changes.'),
'TalkDirectorLifetime':('Game/NPC/TalkDirector.cpp; TalkMessageCtrl.cpp','narrow-game-adaptation','Non-owning talk controller/balloon references must clear on native retirement; move this responsibility to real owner/destructor.'),
'WPadDestruction':('Game/System/WPad.cpp','narrow-game-adaptation','Manual deletion of WPad child controllers belongs native WPad destructor; callers must then stop deleting twice.'),
'WPadOwnership':('Game/System/WPadHolder.cpp; WPad.cpp; tests','delete-facade','Alternative input arena and callback-table publication. Original process already owns input; migrate fixtures and eliminate bootstrap.'),
'XanimeQueryCompat':('Game/Util/HashUtil.cpp','narrow-game-adaptation','Only case-folded resource hash remains; preserve original ASCII C locale and unsigned CP932 bytes without full duplicated lookup table.'),
'JAIAudible':('JSystem/JAudio2/JAIAudible.cpp','restore-original-owner','Original destructor has no native adaptation.'),
'JAIAudience':('JSystem/JAudio2/JAIAudience.cpp','restore-original-owner','Original destructor has no native adaptation.'),
}
rows=[]
for row in json.loads((OUT/'scene-game-scope.json').read_text()):
 p=ROOT/row['path']; dest,disposition,reason=spec[p.stem]
 # Ownership review records are not claims of semantic equivalence or runtime completion.
 rows.append({**row,'destination':dest,'disposition':disposition,'evidence':reason,
              'current_status':'removed' if not p.exists() else 'pending',
              'validation':'See separate batch/build/runtime evidence; classification alone is not validation.'})
(OUT/'scene-game-audit.json').write_text(json.dumps({'scope':'scene/game ownership review','review_level':'Ownership and dependency triage; deeper algorithm validation remains required for pending entries. Not a claim that every method matches retail.','entries':rows},indent=2)+'\n')
all_rows=[]
for lane in ['actor-collision-camera','render-effects','platform-resources','scene-game']:
 obj=json.loads((OUT/(lane+'-audit.json')).read_text()); entries=obj if isinstance(obj,list) else obj.get('entries',obj.get('files',[]));all_rows.extend(entries)
paths=[x['path'] for x in all_rows]
expected={x['path'] for values in json.loads((OUT/'inventory.json').read_text())['groups'].values() for x in values}
assert len(paths)==330 and len(set(paths))==330 and set(paths)==expected,(len(paths),len(expected))
(OUT/'coverage.json').write_text(json.dumps({'inventory':330,'classified':len(paths),'unique':len(set(paths)),'missing':sorted(expected-set(paths)),'removed_currently':sorted(x for x in paths if not (ROOT/x).exists()),'warning':'Coverage is destination planning, not completed removal or runtime verification.'},indent=2)+'\n')
