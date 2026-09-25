# Actual audio wrapper, scene and sound-name owners

Round21 lane A, 2026-09-25. Seven existing Game owner files changed; eight compat files removed. All owned source paths were clean before editing. Exact snapshots, SHA256 values and per-file patches are in `owned-manifest.json`, `before/`, and `patches/`.

## Ownership and behavior

`AudSystemWrapper` directly owns the original `AudSceneMgr`, `AudBgmMgr`, non-owning `AudSoundObjHolder`, system `AudSoundObject`, native sound-name bytes/table, and `AudSoundNameConverter`. It obtains process authority only from the actual `GameSystem` object holder. Absent early/late wrappers and absent real `AudSystem` report disabled output. No replacement service, preview playback state, global publication pointer, limiter array or category-volume sidecar remains.

The original asynchronous SMR file request/receive path and decompressed JKR block bounds are retained. Aurora decodes its existing validated native name table without loading wave banks. The converter now owns its two arrays, including constructor rollback; before publication it validates the three 14/2/1 group sections, u16 item limits, non-null names and s32 total. Names/table storage outlives every converter borrower, and previous actual singleton values are restored on failure/retirement. Short or unknown category names cannot index before the group offsets.

Wrapper children use the actual wrapper heap without a lifetime token back to that heap's finalizer. Its system `AudSoundObject` retains the previous host object storage, because a heap-registered `JKRDisposer` would otherwise be destroyed before the wrapper's finalizer and then deleted twice. Its constructor still runs under the actual selected heap/client allocation scope so original handle/hash arrays belong to that heap. BGM/scene/converter/holder objects have no independent disposer. The wrapper releases the system object before its holder, and releases the converter before its table/bytes. All heap selection scopes cover synchronous construction only.

`AudSoundObjHolder` owns only its original pointer array. Add/remove preserve bounded capacity and maintain the object's actual `mNativeHolder` back-reference, avoid duplicate membership, and detach a previous holder on a successful transfer. Holder destruction clears surviving borrowers' back-references rather than deleting actors' sound objects. Lane B's actual object destructor unregisters through that pointer.

`AudSceneMgr.cpp` keeps its complete donor scene/wave/player tables and algorithms. Physical bank calls are skipped only with a null section heap and explicit disabled output; wrapper initialization/request flags still gate completion. Scene start retains its original local flag reset and resets the actual wrapper's two permission flags before skipping unavailable DSP/effector/speaker work. No voice or hardware completion is claimed.

## Removed files

Both source/header files for `DisabledAudioBackend`, `DisabledObjectAudioService`, `JAudioCategoryVolumeOwnership`, and `JAudioLimitedSoundOwnership` were deleted. Their preview and facade consumers are removed by the other round21 lanes.

## Root integration

Enable the existing full `Game/AudioLib/AudSceneMgr.cpp` and `AudSoundObjHolder.cpp`; converter and wrapper were already compiled. Lane B owns the actual BGM/sound object/JAU dependency closure. Root owns the ordinary Game build glob and explicit wrapper destruction after async producers and scene/actor cleanup, before FileLoader/singletons/heaps retire.

Full donor scene-manager code introduces link references to `JAUSectionHeap::{isWaveLoaded,loadWaveArc,eraseWaveArc}` overloads, `AudSystem::initSceneVolume`, `AudEffector::initParams`, and `SpkSystem::reconnect`; root was notified to provide their canonical disabled-output closure. The physical branches remain preserved and are not reachable under the current output policy.

## Verification limits

Read-only source review checked constructor/destructor order, singleton restoration, original metadata preservation, holder capacity/back-references, snapshot hashes and absence of retired-provider references in src/tests. `source-review.json` records the bounded scan. No builds, runtime tests, fixture additions or Git/index changes were performed by this lane; root owns the integrated check.
