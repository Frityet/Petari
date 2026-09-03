# Original native actor pipeline integration checkpoint

This replaces the native LiveActorModel/ActorAnimationRuntimeState facade with each actor's actual original ModelManager, J3DModel, XanimePlayer, material animations and original DrawBufferHolder/Group/Executer pipeline. Full original ModelManager, MaterialCtrl, DisplayListMaker, ActorAnimKeeper, ActorPadAndCameraCtrl and FixedPosition implementations are active. Complete original utility method bodies supply model, joint, animation, lighting and sound access. The duplicate isEnvelope provider was removed.

ModelManagerOwner retains the actual model/animation archive identities and scene JKR domain. Actor registry entries own animation/camera helpers and actual AudAnmSoundObject instances. Draw executors retain the first model's real material prototype until the holder retires, so destroying an actor cannot invalidate another actor's packets. Original actor lists are allocated once after scene registration; title/File Select constructs its real children before allocation and transfers the same sky owner. SceneJ3dScope preserves original J3D/GD/interrupt state across full scene phases and restores leaked mutex recursion on native exception unwinding.

The native BAS boundary decodes original big-endian BAS resources and retains archive ownership. Original sound animation scheduling runs against non-null original AudAnmSoundObject instances. Object sound playback is explicitly disabled as a whole service; the real RuntimeContext-owned system sound object and handle storage retire before their heap. Existing background-music playback remains in its existing service. No sound IDs are mapped to invented voices.

The full original MarioAnimator construction/update/calc code and MarioActor draw/calc code are selected. MarioAnimatorLifetime retains its actual lower/upper Xanime graph and tables with the actor model domain. It does not implement animation state or timing. Original parts, sound and animation helpers are being selected as their providers close. The original declaration and portable structure fixes come from preceding root-source checkpoints.

## Verified native boundary

Built with Homebrew LLVM23 for arm64 macOS and executed through Aurora Metal on Apple M5 Max with the supplied RMGK01 RVZ:

- `smg-pc-title-file-select-visual-tests`: original sky model with authored BCK/BTK, 21 GX draw calls, retained sky identity through handoff, final retirement.
- `smg-pc-file-select-far-visual-tests`: original FileSelectCameraController, exact step60 far transition, six real planet models and six number layouts, authored StageLight, selection scale progression, full teardown and same-runtime recreation. This test also passes with all65 original scene-connection helpers and the owned disabled object-audio service.

Tests query original model/animation state and actual GX display-copy submission. They no longer inspect the retired native model packet facade. Their links use dead stripping, as the production showcase does, to exclude unrelated unused player methods. The far test now executes ordinary title frames before transition: the source controller publishes its previous watch point. Its original title eye Y is15000; the old test's15800 eye expectation was incorrect (15800 is the watch Y).

## Current incomplete player boundary

This is an integration checkpoint, not a working Gateway jump demo. `smg-pc-showcase` does not currently complete its build. The latest attempt reaches the full original MarioEffect unit and finds missing original effect declarations/owner, a pointer payload still expressed as u32, and the old native void emitEffect interface. A generic pointer-width HashSortTable and actual EffectKeeper/MultiEmitter ownership are being addressed separately. Earlier complete links expose additional original shadow, fur, movement/state and drawing dependencies; no placeholder functions were added to pass the link.

The native Mario constructor and movement loop still contain the earlier walking-slice branches. They must retire together with original state initialization, floor/collision phases, jump/landing and effect owners. In particular, Mario's real sound hash initialization is not yet enabled in that constructor. Do not run partially initialized Mario objects through the original calls or claim that animation speed/jumping has been fixed based on the title tests. No new jump impulse, animation-rate multiplier, actor-name exception or Galaxy-specific compatibility behavior was introduced.

Supporting source/lifetime evidence is in the sibling original-actor-model-retirement, original-actor-animation-lifetime, original-scene-draw-buffers, audio-animation-boundary, original-mario-sound, original-scene-connections, and original-mario-jump-activation notes. Raw local build/runtime logs remain in this directory and are not source artifacts.
