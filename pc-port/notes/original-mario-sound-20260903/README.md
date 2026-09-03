# Original Mario sound owner and disabled system service — 2026-09-03

This package stages the complete original Mario sound table and methods,
original numeric SoundUtil requests, and an explicit runtime owner for disabled
system-object sound. It does not enable the player movement/constructor phase
or claim that Mario jump sound dispatch has executed in live gameplay.

## Package and integration

All native work remains in
`build/original-mario-sound-20260903/staged/`. The `native/` directory here is
the durable frozen snapshot; source/destination changes are also recorded as
diffs. Parent owns native publishing and xmake changes.

Copy these new providers into `pc-port/src/compat/` and add exactly one build
provider for each:

- `OriginalMarioSound.cpp`: complete root table and all 10 Mario sound methods,
  stopping before the unrelated nerve-instance definitions. This owns
  initSoundTable/initSound/playSoundJ/stopSoundJ/startBas/isRunningBas/skipBas,
  Teresa/trample sound helpers and setSeVersion. Do not also compile the full
  MarioSound.cpp or another extraction of these definitions.
- `OriginalObjectSoundRequests.cpp`: eight literal complete original numeric
  actor/system request/stop and map/version methods from root SoundUtil.
- `DisabledObjectSoundRequests.cpp`: the two numeric level request helpers
  preserve original start/condition/return logic. Only the concrete
  `JAISound::updateLifeTime` dereference becomes a native disabled-service
  operation. Disabled object starts return null, so there is no voice lifetime
  to mutate and no fake JAI graph or attached handle is constructed.
- `compat/DisabledObjectAudioService.hpp/.cpp` from the stage: actual scoped
  service owning an `AudSoundObject` member.

Apply the small frozen deltas instead of overwriting concurrently edited
production files:

- `AudioFacadeCompat.cpp.diff`: replace the existing throwing
  AudWrap::getSystemSeObject provider with the borrowed active service object.
  No new duplicate AudWrap provider is added. Existing BGM implementation stays
  in its current owner.
- `GameResourceRuntime.hpp.diff` and `.cpp.diff`: expose the existing retained
  process JKR heap owner through `host_heaps()`.
- `RuntimeContext.hpp.diff` and `.cpp.diff`: own a unique DisabledObjectAudioService
  beside the resource/audio members; construct it from resources.host_heaps()
  and retire it after scene actors/scheduler/capture objects have retired.

The only root Game source changes are the two portable layout corrections in
`src/Game/Player/MarioSound.cpp`, also captured by `MarioSound-root.diff`:
read the original high flag byte numerically (`_0 >> 24`), and address the
three swap columns through a typed `u32 offsets[3]` array. The Wii struct layout
and valid column selection are preserved; native code no longer interprets a
host pointer as one 32-bit table word. Root changes precede native staging.

Native include priority must be pc-port overrides, Aurora SDK, then original
root Game/JSystem fallback headers. The stage compiler pins that order. All
includes remain portable (`JSystem/JAudio2/JAISound.hpp`); Aurora's actual token
handle ABI is used. No replacement sound graph/header was added.

## Ownership

DisabledObjectAudioService retains a shared `JkrHeapRuntime` and constructs its
actual AudSoundObject against that explicit root heap. It owns the object as a
member; reverse member destruction disposes the object/handles before releasing
the retained heap. `make_disabled_object_audio_service` uses a host-allocation
scope so creating the service inside a transient Game heap scope cannot put
the service itself on that transient heap.

Only a borrowed thread-local active pointer is published. Construction saves
the previous owner, destruction restores it, and querying never constructs an
object. RuntimeContext explicitly retires this service before resource/heap
teardown, including its constructor-unwind path. There is no function-static
AudSoundObject, lazy allocation in getSystemSeObject, or exit-time heap user.

## Activation dependency

The current native Mario constructor sets `_96C = nullptr` and skips original
`initSound()`. Real `playSoundJ`/`stopSoundJ` require the original HashSortTable
created by that initialization. Restore original initialization with the real
constructor phase; adding these link providers alone does not initialize it.
Do not substitute a null-return guard. The full constructor/state frontier is
documented in `original-mario-jump-activation-20260903/README.md`.

`skipBas` also retains its original non-null sound-owner precondition. The
parent's actual actor sound initialization must supply that owner. The existing
general disabled object backend satisfies the precondition without playback.

## Verification and scope

- Complete root MarioSound compiles with the original GC/3.0a3 configuration.
  `startBas` (55 instructions), `isRunningBas` (15) and `skipBas` (3) match every
  fully relocated byte of the supplied RMGK01 DOL. All three also report 100%
  objdiff. The existing table/dispatch bodies remain fuzzy-only at
  99.0/97.77273/98.01588% for initSoundTable/playSoundJ/stopSoundJ.
- All seven staged native translation units compile, including the complete
  staged RuntimeContext, GameResourceRuntime and AudioFacade providers.
- Eight Mario sound/hash/BAS/version methods are explicitly retained in the
  native probe link, so their request/provider references are resolved even
  though the probe does not construct a partial Mario object. This is link
  proof for those Mario methods, not execution proof for playSoundJ or Mario BAS.
- Runtime probe executes the actual original numeric actor/system requests,
  stops, map-gravity and version setters using real LiveActor/AudAnmSoundObject
  owners. All six requests reach the disabled service and decline consistently.
- Runtime probe verifies that a service created inside a transient Game scope
  stays on the host heap, its SDK handles belong to the retained process root
  heap, scoped publication restores the previous owner, and retirement clears
  publication before process teardown.
- The earlier real BAS probe also runs against the currently integrated
  production providers: 39 BAS files / 141 events decode and schedule, and full
  original MR::startBas works through actual ModelManager/ResourceHolder and a
  non-null disabled AudAnmSoundObject.

The probe link has a pre-existing unrelated duplicate MR::isEnvelope warning
from the concurrently integrated model providers. It has no duplicate
AudWrap::getSystemSeObject warning after the facade replacement. No shared
xmake build or GPU test was run by this task.

Artifacts: `compile-evidence.json`, `retail-evidence.json`, `link-evidence.json`,
`runtime.log`, and the isolated build's link map. Retail DOL SHA1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`.

```sh
python3 pc-port/notes/original-mario-sound-20260903/stage.py
python3 pc-port/notes/original-mario-sound-20260903/verify-retail.py
python3 pc-port/notes/original-mario-sound-20260903/verify-link.py
```

The stage is a snapshot of the reviewed native files. Rebase its small diffs
when production files change; do not overwrite later parent work with whole
snapshot files. Existing library archives must be present for the isolated
link command; rebuilding those shared archives remains the parent's task.
