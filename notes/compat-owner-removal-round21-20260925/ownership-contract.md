# Next batch: delete all 24 audio compat files

Read-only integration contract, 2026-09-25. Keep audio output explicitly disabled. Preserve real sound-name conversion, original Game audio objects/fields, animation state, async initialization and teardown. Remove preview PCM playback and its event simulator; retain Aurora's reusable mixer/BAA/STRM decoders. No new service, global facade, singleton replacement, or wrapper-shaped sidecar.

## Shared API contract first

Lane A adds these methods directly to **Game/System/AudSystemWrapper**:

```cpp
static AudSystemWrapper* getCurrent() noexcept;
static bool isOutputDisabled(); // existing public policy query
AudSceneMgr* getSceneMgr() const noexcept;
AudBgmMgr* getBgmMgr() const noexcept;
AudSoundObject* getSystemSeObject() const noexcept;
AudSoundObjHolder* getSoundObjHolder() const noexcept;
void setTriggerSePermitted(bool) noexcept;
void setLevelSePermitted(bool) noexcept;
bool isSePermitted() const noexcept;
```

`getCurrent()` reads only `SingletonHolder<GameSystem>::get()->mObjHolder->mAudioSystem`, with null checks and no allocation/publication. It is not a new global pointer. Instance accessors return the actual typed children owned by that wrapper (null before initialization or after retirement). Existing wrapper initialization/load/reset methods remain the lifecycle API. No playback, stream, category-volume or limiter service accessors.

Wrapper state is directly typed: owned AudSceneMgr, AudBgmMgr, AudSoundObjHolder, system AudSoundObject, decoded name bytes, JAUSoundNameTable(false), AudSoundNameConverter, plus initialization phase/bank-request/reset flags and two SE-permission flags. Store actual fields/unique pointers, not a `NativeAudioState` service. The disabled process still has no AudSystem, rhythm graph, DSP thread, or fabricated output voice. System sound object/holder may use the donor handle count 10 and `AudParams::numInspectableSoundObj`; starts fail before accessing a sound starter.

Lane B's canonical AudWrap uses `getCurrent()` to obtain these four owners. `getStageBgm/getSubBgm` inspect that real BGM manager; starts return no handle under disabled output. Missing AudSystem/remix/rhythm access remains an explicit unsupported-owner boundary instead of returning a fake object. `getSoundInfo()` may return the absent real singleton, as original callers already check it. No process-global BGM manager, event override, preview lookup or fallback allocation.

SoundUtil permission functions use the three wrapper permission methods, preserving submit/permit/query state. With no output, limitedSound and category-volume setters explicitly do nothing after normal name resolution where applicable; no synthetic limiter/category arrays survive. The complete original AudSystemVolumeController remains available for a future real AudSystem and is not constructed with null system merely to preserve its old sidecar.

## Lane A — process wrapper, names, and actual holder

Own `Game/System/AudSystemWrapper.{hpp,cpp}`, `Game/AudioLib/{AudSoundNameConverter,AudSoundObjHolder,AudSceneMgr}.{hpp,cpp}`. Delete eight files: `compat/{DisabledAudioBackend,DisabledObjectAudioService,JAudioCategoryVolumeOwnership,JAudioLimitedSoundOwnership}.{cpp,hpp}`.

- Keep existing `/AudioRes/SMR.szs` async request/receive and bounded JKR allocation-size validation. Use existing Aurora JAudioSoundArchive decoding to obtain native sound-name bytes without wave-bank loading. Decode before removing the FileLoader allocation.
- Preserve the old name-owner checks: exactly three sections with 14/2/1 groups, each item count below 65536, names non-null, aggregate count fitting s32. The original converter uses u8/u16 loops and fixed 17 offsets. Publish the real JAUSoundNameTable and converter only after validation, restore previous singleton values on failure/retirement, and retain bytes until after converter destruction.
- Add actual converter destructor/constructor rollback for its two arrays. No manually deleting its internals from an outer service. Ensure allocations use the intended actual original heap but do not retain that heap back through its own finalizer.
- Enable full AudSceneMgr donor. Preserve scene/player flags and original scene metadata tables. With null section heap under explicit disabled output, wave requests have no physical work; wrapper request flags and initialization gate determine completion. `startScene` resets `_4`, `_1D` and wrapper permission flags, then skips absent DSP/effector/speaker work. It must not dereference AudSystem.
- Add actual AudSoundObjHolder destructor and registration back-reference maintenance. Its list is non-owning: free the array, detach surviving members, never delete borrowed actor sound objects.

**Header coordination with lane B:** add `AudSoundObjHolder* mNativeHolder = nullptr` to AudSoundObject. A's holder add/remove sets/clears this pointer (handle prior registration without duplicates); its destructor nulls members' pointer. B's object destructor removes through this exact pointer, then deletes mHashDatas. It never calls AudWrap during destruction. B owns AudSoundObject.hpp/.cpp; A owns holder files only.

Lifecycle order: stop/join async producers, destroy scenes and actor resources, explicitly destroy the wrapper while GameSystem/JKR heaps are still alive, release its system sound object before holder, release converter before table/bytes, then FileLoader and process heaps. Existing wrapper finalizer is unregistered by explicit destruction. Remaining heap-disposed sound objects safely have null holder back-references. No voice drains are needed because there are no created voices.

## Lane B — complete donor Game/JAU algorithms and disabled starts

Delete six files: `compat/AudioFacadeCompat.{cpp,hpp}`, `DisabledObjectAudio.{cpp,hpp}`, `OriginalObjectSoundState.cpp`, `OriginalAudioVolumeController.cpp`.

Enable existing complete sources:

- `Game/AudioLib/{AudFader,AudTrackController,AudBgm,AudBgmKeeper,AudBgmMgr,AudBgmRhythmStrategy,AudWrap,AudSystemVolumeController}.cpp`.
- `Game/AudioLib/{AudSoundObject,AudAnmSoundObject,AudSoundObject_Kawamura,AudSoundObject_Takezawa,AudSoundObject_Gohara,AudSoundInfo,AudUtil,AudSpeakerWrap}.cpp`.
- Import complete `decomp/src/JSystem/JAudio2/{JAUSoundObject,JAUSoundAnimator}.cpp` to canonical src/JSystem owners, reconciling donor headers from `decomp/libs/JSystem/include/JSystem/JAudio2/` with existing native handle/pointer fields.

Preserve original algorithms. Narrow disabled-output starts belong on AudBgmMgr::start and direct AudSingleBgm/AudMultiBgm starts, AudSoundObject::isEnableStartSound/isLimitedSound, and actual ME/speaker owners. Guard *before* any absent AudSystem/starter/rhythm dereference. The donor AudSoundObject::isEnableStartSound is inline in its header; native header currently declares the compat implementation, so reconcile it deliberately. Full modifier sources supersede the two-ID parameter whitelist. AudUtil interpolation is a cheap existing donor dependency and must be enabled.

Restore full JAUSoundAnimator/AudAnmSoundObject scheduling, retaining `resolve_bas_animation()` at startAnimation and original native BAS layout. JAUSoundObject start methods may return absent handles when their actual JASGlobalInstance starter/SE manager is absent; JSystem must not depend on Game's wrapper policy. Keep constructor/handle storage and destruction real. AudSoundObject frees its own hash array and uses the holder back-reference described above.

Also own `Game/Util/SoundUtil.cpp` and removal of DisabledObjectAudio policy imports in `Game/Screen/{GamePauseSequence,THPSimplePlayerWrapper}.cpp`, `Game/Scene/GameScenePauseControl.cpp`, `Game/System/GameSystemErrorWatcher.cpp`. Use actual wrapper policy (runtime condition replacing old if-constexpr). THP keeps `mAudioExist=false` under disabled output. Restore/enable the existing `RhythmLib/AudMeObject.cpp` and `AudMeHandles.cpp` only as needed to own the prior startMe stub; no new mixed provider.

Audit direct absent-system calls in `Game/Scene/GameSceneScenarioOpeningCameraState.cpp` (`set830`) and `Game/Screen/HomeButtonLayout.cpp` (menu/reset audio). Skip only their audio operations when disabled. `AudMicWrap`/AudCameraWatcher/AudEffectDirector already contain policy guards; keep them. Do not invent a listener position if getMicPos is requested without its real owner.

## Root — preview deletion, canonical low-level providers, and wiring

Delete ten files: `compat/{NativePcmSound,JaiStreamPlayback,JAudioSoundParameterSemantics}.{cpp,hpp}` plus `compat/jaudio/{JasStreamPcmBackend.cpp,JasStreamPcmBackend.hpp,JasAramStreamPlatform.cpp,JasGenericPoolPlatform.cpp}`. Delete `runtime/JAudioPlaybackService.{cpp,hpp}`. Remove all related RuntimeContext constructor injection, members, frame/shutdown work, playback/query methods; remove AudioEventService/enums and its ParityTrace serialization from RuntimeServices. All non-Game production consumers found are in this runtime/trace branch.

Add explicit wrapper destruction in `app/OriginalGameApplication.cpp` after scene/actor cleanup and before FileLoader/singleton release. Current wrapper finalization otherwise occurs **after** `SingletonHolder<GameSystem>::release()`, which is why the back-reference and explicit ordering are required.

Restore canonical `JSystem/JAudio2/JASHeapCtrl.cpp` and `JASReport.cpp` from existing donors; preserve pointer-width-safe pool links, original arrays and heap boundaries. Delete preview-only `retire_jas_pool` and weak global pool owner. Restore canonical JASAramStream with explicit unavailable-output operations, not an active_backend map. Any remaining link-only JASTrack/JAISe/Seq/rhythm/DSP functions can be explicit disabled stubs on their actual owners for this batch; do not construct fake managers or report a sound as attached. Import cheap complete existing donors where practical. A full DSP/JAU initializer bring-up is not this task.

Root updates `src/Game/xmake.lua`: remove exclusions for the 18 AudioLib sources listed across A/B (A has SceneMgr/SoundObjHolder, B has 16), add the four canonical JSystem owners JAUSoundObject/JAUSoundAnimator/JASHeapCtrl/JASReport and chosen JASAramStream provider, and only enable additional ME/rhythm units needed for symbol closure. Existing Game files are already imported; copy only missing JSystem sources/headers from decomp. Preserve existing native files and CP932 literals; use -ffp-contract=off for restored scalar fade/volume/modifier arithmetic. Compat glob removes deleted providers automatically.

## Exact existing fixture scope

- Delete `tests/JAudioPlaybackTests.cpp` and target `smg-pc-j-audio-playback-tests`: preview voices/override/event semantics intentionally retired.
- Delete `tests/OriginalAudioCategoryVolumeTests.cpp` and target `smg-pc-original-audio-category-volume-tests`: exclusively sidecar controller/category tests.
- Reduce `tests/OriginalJaiSoundOwnershipTests.cpp` to its existing JAISoundID bit-layout and JAISoundStatus flag/prepare-transition assertions at the beginning of `test_native_and_stream_owners`. Remove all six service/backend/name-owner/talk/PCM/stream fixture functions and old command-line modes, fixture file I/O and deleted includes. Keep target `smg-pc-original-jai-sound-ownership-tests` for these existing actual SDK checks; no new fixtures/assertions. The limiter function is itself sidecar-based and goes too.
- Keep Aurora audio decoder/mixer tests unchanged. No new tests proposed and none run for this plan.

All source remains frozen; this file is the only change from this audit. Agree the wrapper API and holder back-reference before parallel implementation, then root builds the complete deletion batch rather than individual incomplete lanes.
