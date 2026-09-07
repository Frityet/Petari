# Original scene wipe owners — 2026-09-07

The native scene now constructs the complete original SceneWipeHolder and its five actual descendants: ring, black fade, white fade, GameOver, and Koopa. All six original TUs and seven headers are copied byte-identically from decomp. Parent-owned StageInitializationService creates SceneWipeHolder immediately after the original IgnorePauseNameObj group. No Gateway-specific setup, substitute wipe state, or absent-resource success was introduced.

`root-manifest.json` lists this cohort's27 paths and SHA256 values; `decomp-manifest.json` lists the two reference corrections. The parent owns StageInitializationService/StageHost integration and its manifest. tests/xmake.lua is shared with the independently prepared PLAY fixture; both appends are preserved. No Aurora changes belong to this cohort.

Reference corrections were recovered before native copying using actual retail objects under notes/gateway-audit-20260907/restoration/retail:

- SceneWipeHolder's black fade name is フェードワイプ, distinct from 白フェードワイプ. The old source used the white label twice. Corrected constructor99.695%, with actual data-string comparison documented separately.
- WipeRing::getMarioCenterPos preserves the original IceVolcano/scenario/proximity exclusion and SuddenDeath predicate; historical source reversed branches and omitted the distance branch. Restored99.5%,216bytes. These authored conditions are original Game behavior, never native special cases.
- WipeRing::calcMaxRadius is MR::sqrt(900160), replacing the historical reciprocal-square-root error. Restored98.75%,64bytes.
- All six TUs compile with the Wii compiler. The table contains108 paired function entries (including repeated compiler helpers),91exact and105>=90%. Three lower entries are existing template/destructor compiler differences, with no hidden method omission.
- All17 scene wipe ScreenUtil wrappers are copied unchanged; every one matches retail100% in utility-function-proof.json. Process SystemWipe/Capture ownership is outside this scene cohort.

Native lifetime and animation boundaries:

- MR::isAnimStopped now has its exact original const LayoutActor pointer declaration. The redundant obsolete SimpleLayout overload is removed. The native owner validates the initialized manager, actual root pane control and layer before observing its animation. A real initialized player with no transform is stopped, exactly as original LayoutAnmPlayer::isStop; missing owners and invalid layers still reject. Named pane controls retain the same default-state and explicit rate-zero/state1 behavior.
- Native NameObj retirement compacts every other actual live NameObjGroup array before erasing the retiring registry entry. It removes all borrowed occurrences, preserves survivor order/duplicates/capacity, and clears trailing slots. Scanning the real objects covers original direct LiveActorGroup::registerActor calls as well as MR joins. It makes no allocation or callback. The retiring object itself is skipped because a group's own derived destructor has already released its array.
- Scene construction failure can therefore roll back joined children while leaving a valid surviving group. No changes to original NameObjGroup registration/destruction code are needed.

Validation currently complete:13/13 integration TUs pass isolated LLVM23 syntax. All21 active existing Game callers of isAnimStopped also compile; the excluded reference-only Util/LayoutUtil.cpp cannot compile because its preexisting ResourceHolderManager header is absent and is not selected in the Game archive. That excluded failure remains recorded in const-animation-caller-syntax.json. Wii/native wrapper probes and all13 original mirror byte comparisons pass. Full native target builds and runtime outcomes are pending parent coordination; this is not a wipe rendering or gameplay success claim.

Prepared focused targets:

- smg-pc-name-obj-group-lifetime-tests:32 repeated cycles of multiple/duplicate memberships, original derived LiveActorGroup registration, group-before-member destruction, actual captured scene factory rollback, original pauseOffAll after retirement, and successful recreation in the same scene.
- smg-pc-original-scene-wipe-owner-tests: actual RVZ resources and typed identities/order for all five descendants; const/root/named-pane stopped predicates; missing owner/layer rejection; original fade frame transitions and kill boundary; actual Koopa and GameOver BRLAN-driven nerves; and two complete scene lifetimes with registry and Game-domain retirement.

Evidence: integration-syntax-results.json, const-animation-caller-syntax.json, restored-wii-results.json, function-proof.json, utility-compile-results.json, utility-function-proof.json, reference-corrections.md, layout-animation-boundary.md, and name-obj-membership-design.md.

Pre-link duplicate correction: the old active native Game/Util/ScreenUtil.cpp defined15 substituted scene leaves. Its remaining system/capture host functions were relocated unchanged into compat/ScreenSystemAndCaptureCompat.cpp; the old15 scene wrappers/helper were removed, Game/Util/ScreenUtil.cpp restored byte-identical reference, and that reference TU explicitly excluded. The17 new exact scene wrappers are the sole active definitions. The shared archive build then passed; fixture links reached two original environment queries. Those exact decomp bodies are now provided by OriginalScenePredicates.cpp (native/Wii compile0; isExistMario100%, stage predicate90.227%). Runtime reruns remain pending.

The independent timer-based native scene WipeService is still consumed directly by DemoSceneRuntime and GatewaySpinCheckpoint, with traces reporting its duplicate state. Exact consumers and the required actual-owner migration are listed in native-wipe-consumer-audit.md. That migration is held until the shared build finishes; process SystemWipe ownership remains a separate frontier.
