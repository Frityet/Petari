# Original Player animation owner activation, 2026-09-07

MarioActorEye.cpp, MarioAnimationEfx.cpp and MarioModule.cpp now compile whole in
the normal Game archive. Their showcase-only entries are removed to retain one
provider per original symbol. All three native sources are byte-identical to
current decomp sources (source-parity.json). No new animation-specific compat
stubs were introduced.

MarioModule::isInputDisable now uses its original full body: movement-disabled
flag, first-person-view status, four authored animation guards and the actor
flag. The native shortcut had discarded the status and animation tests.

Two animation-attribute methods previously recovered before flattening were
retained only in native source after the upstream merge. They were restored to
decomp first, preserving their original bodies and the prior documented proof
in ../original-mario-nonstop-20260903/. Fresh PPC builds again give each100%.

The scaled addVelocity had a native multiply-then-add expression while decomp
contained only a comment documenting the inlined MR::vecScaleAdd call. Restored
that canonical original helper call in decomp and copied the complete file to
native. Retail44bytes load the velocity and input before two paired fused
multiply-add operations and stores. The out-of-line C++ call is16bytes and scores
25.45% because the helper is no longer inlined. Its functional behavior delegates
to the original helper; no assembly was added to the recovered Game method.
The parent added the generalized native helper with load-before-store alias
safety and explicit std::fma, including exact fused-versus-unfused rounding and
self-alias regressions. This preserves behavior that the old native expression
could lose.

Fresh configured MW/GC3.0a3 builds pass for all three complete source files;
retail object comparisons give .text99.09% for MarioActorEye,98.91% for
MarioAnimationEfx and96.72% for MarioModule. Comparing a freshly compiled prior
MarioModule baseline shows no existing symbol score regression: NonStop,
WithAttr and scaled addVelocity move from absent symbols to supplied functions.
isInputDisable remains99.58%. Commands, hashes, disassembly and full objdiff
reports are retained beside this note. Native LLVM23 syntax passes all3TUs.

The first native activation build resolves the four initial animation symbols.
Math rotation/fused-alias target builds and passes. Talk and factory then expose
seven real dependencies through the original callback table: three actor effect
methods, three animator methods and gIsLuigi. Factory required one missing
LiveActorUtil.hpp include for the parent's new collision invalidation assertions.
No collision-specific undefined symbols remain in that build. The parent coordinates root/decomp staging and commits.

Changed paths owned by this task:
- decomp/src/Game/Player/MarioModule.cpp
- src/Game/Player/MarioModule.cpp
- src/Game/xmake.lua (only these three exclusions)
- src/showcase/xmake.lua (only these three duplicate entries)

MarioActorEye.cpp and MarioAnimationEfx.cpp require build-list activation but no
source modifications. Earlier talk/shadow changes remain separate checkpoints.

## Expanded complete owner activation

The dependency closure now selects complete MarioActor.cpp, MarioAnimator.cpp
and MarioEffect.cpp in the normal Game archive and removes their duplicate
showcase entries. MarioActor::isAnimationRun's copied compatibility definition is
retired from MarioCameraAccessCompat.cpp; the original data owner supplies
actual gIsLuigi. No alternate global or partial callback table is introduced.

All three current decomp files compile freshly with the configured Wii compiler:
.text92.74% Actor,94.83% Animator,93.47% Effect. All three native TUs pass isolated
LLVM23 syntax. No Game bodies in this expanded set were changed by this tranche.
Native Effect's differences are packed-byte endian access; Animator retains its
existing lifetime scope, real weight array, typed joint field and compile scope.
MarioActor still has existing walk-slice initialization and gameplay suppression
branches; selecting the whole TU does not restore those original gameplay paths.
The next movement/camera work must address them separately. This limitation is
explicitly recorded rather than claiming complete original MarioActor behavior.

Additional edited paths: src/compat/MarioCameraAccessCompat.cpp (duplicate
method removal) and tests/NameObjFactoryPlacementTests.cpp (one required include).
The second native link validation awaits the backend lane's focused depth test.

## Expanded native validation

The second complete-owner build removes all seven callback-owner links. Talk
and factory now reach three further existing original-owner methods only:
FloorCode::getCode (MarioMapCode.cpp), MarioActor::getModelData
(MarioActorDraw.cpp), and Mario::getAirGravityVec (Mario.cpp). Their complete TU
activation is held for the parent's coordinated checkpoint. StarPointer's
existing disconnected-device boundary test builds/runs0. Its success covers
those tested device states, not creation/lifetime of a full StarPointer target.

Fresh showcase compiles but links with five utility dependencies:
isNearZero(TVec2f,float), initFurPlayer, startDPDHitSound,
createAreaPolygonListArray and isInitializeStatePlacementSomething. Parent owns
that closure. Exact unresolved reports and build logs are retained with expanded
in their filenames. No showcase execution or completed Gateway demo is claimed.

## Final three source owners selected

After checkpoint5bbd3c998, Mario.cpp, MarioActorDraw.cpp and MarioMapCode.cpp move
from the showcase-only list into the normal Game archive. No extracted method
definitions overlap these units. No Game body was modified for this activation.
Fresh native syntax passes all three. Current Wii requested methods compare at
99.64% getAirGravityVec,100% getModelData and100% FloorCode::getCode. Whole-source
Mario .text60.93% and ActorDraw89.38% expose existing decomp incompleteness;
MarioMapCode is100%. These source scores are not a full original gameplay claim.

Existing TARGET_PC branch inventory is saved in
existing-player-divergence-audit.json. Some branches are architecture/compiler
repairs or previously recovered code; they must not all be removed blindly.
Verified behavior replacements that require future owner activation include:
Mario construction skips original sound-table initialization and leaves37 state
owners null; initAfterConst skips Move/Foo/Swim initialization; the update loop
uses a custom stand/walk and grounding/release path; MarioActor replaces its
original movement/init2/initAfterPlacement/control paths. This activation does
not repair these preexisting gameplay substitutions. The parent's next movement
and camera work must restore original owner behavior through generalized native
boundaries.

## Coordinated native validation with all nine owners

The final three owner activations resolve every Talk/factory Player link. All
seven regression targets compile and link; factory placement, actual StarPointer
target ownership, initialization phases, SceneObjHolder and the fresh two-scene
original effect ownership test run successfully against the Korean RVZ. The
latter again reports seven live particles, nine real GPU draws and 2,391,153
changed RGBA bytes per scene, with callback, orphan retirement and heap checks.

Two fixture failures remain in the first run and are recorded without hiding
those results: the new type3 talk case incorrectly used a utility that retail
explicitly rejects while a timekeeper is active; the LensFlare placement fixture
omitted the now-required real resource service. Both fixture corrections now build and run successfully on the Korean RVZ
without changing the production guards. Final-native-validation-results.json
records that initial run; follow-up fixture results live beside the Talk query
notes.

The showcase compiles and now has only two unresolved functions: initFurPlayer
and createAreaPolygonListArray. Both independent subsystem closures are assigned
to other lanes. No showcase execution or complete Gateway progression is claimed.
