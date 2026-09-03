# Disabled object/animation audio boundary

This is a staged native boundary, with one root-first decompilation recovery.
It does not activate audio, modify the shared build, or claim full gameplay.
All native source changes remain in
`build/audio-animation-boundary-20260903/staged/` for parent review.

## Verified result

The isolated native probe constructs an actual `AudAnmSoundObject` with detached
Aurora handles allocated from an actual original JKR heap. It runs literal
`MR::startBas` on a constructed `LiveActor`, the original default `ModelManager`,
and a real `ResourceArchiveOwner` loaded from `MarioAnime.arc`. The ModelManager
is bound to that holder for this resource-access test; the probe does not create
or claim to exercise a rendered model graph.

`beecreepwalk.bas` resolves to two native events, frame 30 advances to event 1,
and starting a missing BAS clears the prior animation. The non-null sound object
reaches the original foot-code query and map-code assignment. The actor has no
binder in this test; positive/negative/extra map-code state is tested separately.

The probe additionally passes forward, reverse, loop, and start-position
scheduler cases; retained-source retirement; six playback entrypoints returning
null without throwing; detached handle and absent sound-graph checks; and 39
real Mario BAS files containing 141 events. It advances the real files in half
frame steps and checks decoded sound IDs and scheduler bounds. This avoids the
incorrect assertion that jumping directly to frame 1000 must consume lifetime
events whose active interval was skipped.

The final PowerPC proof fully relocates all calls and compares all 57 instructions
of `MR::getMapSoundCodeFoot` against retail at `0x803FA720`, size `0xE4`.
Every byte matches the DOL with SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`. The root recovery is in
`src/Game/Util/SoundUtil.cpp`, including required Binder and MapUtil headers.
It preserves retail's ground/else-wall/else-roof pointer checks and result
preference; no binding or collision behavior was invented.

## Boundary and exact files

`DisabledObjectAudio.cpp/.hpp` implements one explicitly disabled service for
the entire JAU/Aud object-audio subsystem. All object sound-start entrypoints
decline playback, and queries report no active sound. There are no actor names,
stage names, fabricated `JAISound` objects, virtual audio tracks, or new handle
ABI. BGM remains the existing independent PCM service.

The provider keeps ordinary detached Aurora handle arrays for actual objects.
`JAUSoundHandles::getSound` and `JAUSoundAnimator::getSound` return null because
this subsystem never starts an SDK sound graph. External code must not attach
unrelated PCM backend voices to these object-owned handles. Graph-only aging,
parameter-port, and lifetime operations are inert at this disabled boundary.
This does not purport to implement enabled object audio, name-to-sound mapping,
voice priorities, per-sound Game remapping, or positional mixing.

`OriginalObjectSoundState.cpp` imports original Game skip, update, start-position,
parameter-clamp, and map-code methods, plus original JAU event indexing and
parameter/scheduling methods. Native `startAnimation` first resolves the BAS
identity; its remaining scheduling body is original. Disabled `updateAnimSound`
uses the original `skip` method, preserving its exact forward/reverse/loop
progression without traversing an absent sound graph.

`OriginalActorSound.cpp` contains literal original `MR::startBas`,
`MR::actorSoundMovement`, and the recovered foot-code method. None of these MR
bodies contains a disabled-audio shortcut. It replaces the earlier staged
`OriginalActorBasStart.cpp` provider and must not be linked alongside it.

Headers are staged under their original include paths:

- `Game/AudioLib/AudAnmSoundObject.hpp`: byte-for-byte root copy.
- `Game/AudioLib/AudSoundObject.hpp`: original members/methods; move only the
  inline `isEnableStartSound` implementation out of line and remove its two
  now-unneeded full-system includes.
- `JSystem/JAudio2/JAUSoundAnimator.hpp`: original members/methods; move the
  incompatible `handle->getSound()` inline body out of line.
- `JSystem/JAudio2/JAISoundHandles.hpp`: same inline extraction plus a forward
  declaration for `JAISound`.
- `JSystem/JAudio2/JAUSoundObject.hpp` and `JASSoundParams.hpp`: unchanged root
  headers. Aurora's `JAISound.hpp` and `JAISoundHandle` remain unchanged.

`BasResource.cpp/.hpp` retains the archive, bounds-checks and decodes the
big-endian count and 32-byte event records, and creates a native
`JAUSoundAnimation` with a concrete original `JAUSoundAnimationControl` to own
the variable-length event array. It registers both raw and native identities
for the owner's lifetime. Invalid/truncated resources, serialized control
pointers, nonfinite values, and zero interval divisors fail at resource load;
ordinary valid sound-start requests never throw merely because audio is disabled.

`ResourceHolderCompat.cpp` differs from the current provider only by adding the
BasResource include, a BAS storage-kind case, a retained vector, and BAS source
registration. The original ResourceHolder still stores its original raw identity
in the original table. Both original Game consumers, `MR::startBas` and
`Mario::startBas`, pass this identity into `JAUSoundAnimator::startAnimation`,
where the native resource boundary resolves it before any native dereference.

`OriginalSoundModelAccess.cpp` is probe-only literal constructor/accessor closure.
It must not be published alongside the full original ModelManager/actor-access
providers. Parent has since added production `compat/OriginalActorResource.cpp`
for the two resource accessors; production activation must select exactly one
provider for each of those symbols. No ModelManager implementation was invented
for the audio probe.

## Integration requirements

1. Publish the audio headers and providers atomically, with the BAS retained
   resource registration. The raw disc layout cannot be read as a native
   pointer-bearing JAUSoundAnimation.
2. Restore original `LiveActor::initSound` through the normal native ownership
   lifecycle. The disabled classes support both a real position pointer and
   the original null position for 2D sound objects. A null sound slot is not the
   only supported branch. Ensure the sound object is destroyed before its
   allocation cohort/resource backing is released.
3. Retire overlapping BAS and ModelManager resource accessor extraction units.
   Root BCK wrappers can call the literal MR provider without actor checks.

## Reproduce

From the repository root, after reviewing the sources:

```sh
python3 build/audio-animation-boundary-20260903/stage.py
python3 build/audio-animation-boundary-20260903/verify-retail.py
python3 build/audio-animation-boundary-20260903/verify-native.py
python3 build/audio-animation-boundary-20260903/link-native.py
build/audio-animation-boundary-20260903/probe build/original-resource-holder-20260903/MarioAnime.arc
```

The native compiler arguments are taken from the current ResourceHolder entry
in `compile_commands.json`. Linking reads the existing resource-holder test's
flags, uses existing static archives, and supplies the installed Abseil 20240722
libraries needed by the already-built Aurora input object. No shared target is
built or edited. The existing package libraries emit macOS 26.5-versus-26.0
deployment warnings; the link succeeds.

Evidence here includes source hashes, exact compile/link commands, the final
runtime log, retail proof, literal-source imports, and review diffs. The probe
is an isolated boundary test; production actor/BCK activation remains parent work.
