# Original CameraTargetPlayer source import

This tranche supplies the original target implementation for a persistent
runtime owner to use. `src/compat/OriginalCameraTargetPlayer.cpp` contains all
24 `CameraTargetPlayer` methods and the original local zero vector from root
`src/Game/Camera/CameraTargetObj.cpp`. `CameraTargetObj` construction remains
owned by `src/compat/CameraLocalUtilRuntime.cpp`.

`src/compat/MarioCameraAccessCompat.cpp` supplies 24 additional unchanged
accessors: eleven `MarioAccess` methods, eight MR player wrappers, and
`Mario::getCameraCubeCode`, `Mario::isSwimming`, `Mario::isRising`, and
`MarioActor::getGravityInfo`, plus `MarioActor::isAnimationRun`. Their full root source units are not enabled in
the native player build. Existing providers for gravity vectors, status
queries, base target construction, and the MR player matrix remain separate;
none were duplicated. The existing compatibility source glob includes the
two new files without build configuration changes.

The bodies preserve bound-target axes, Bee gravity queries, shadow position,
normalized up, area/ground identity, and demo movement-timer behavior. The
target methods and player accessors were compared byte-for-byte with
root source. `source-correspondence.json` records original paths, lines,
whole-file SHA256 hashes, and individual method-body SHA256 hashes. Run
`python3 pc-port/notes/original-camera-target-player-20260903/verify-source.py`
from the repository to repeat the check. This is source correspondence,
not new binary matching or runtime validation. The later Xanime query import
has two explicit adaptations, described below and verified as recorded
transformations rather than counted as unchanged source.

The first integration link exposed the actor animation-query forwarder as
missing: its mirrored source at `src/Game/Player/MarioActor.cpp:609` lies
inside the retail-only branch from lines 543 through 789. The unchanged root
body at `src/Game/Player/MarioActor.cpp:480` now forwards from the compatibility
unit. Its inherited dependency is `MarioModule::isAnimationRun`, already
compiled by the showcase player slice from `MarioModule.cpp:105`, which queries
the original Xanime players. This restores the water-mode animation check
without altering the query or returning a fixed mode.

## Xanime query link closure

`src/compat/XanimeQueryCompat.cpp` restores sixteen methods: original
`XanimePlayer::isRun` and `getSimpleGroup`, seven XanimeResourceTable lookup
methods, four ResTable lookups, `HashSortTable::search(u32,u32*)`,
`MR::extractString`, and `MR::getHashCodeLower`. They retain current-group
identity and simple-motion resource identity comparisons. Existing
`MR::getHashCode` and `MR::strcasecmp` remain their sole providers.

Two bounded changes are recorded in the manifest: braces scope the initialized
local in `getGroupInfo`'s case 1 so Clang accepts the switch; lower-case resource
hashing indexes the original MSL `__lower_mapC` using an unsigned byte instead
of using the host locale or signed-char classification. The exact 256-byte
table is copied from `libs/MSL_C/source/ctype.c:14`, which is also hashed and
verified. The query import adds no constructors or animation state. The
verification script covers 64 method bodies: 62 unchanged and these two
recorded adaptations.

The active PC `MarioAnimator::init` still sets `mResourceTable`,
`mXanimePlayer`, and `mXanimePlayerUpper` to null
(`src/Game/Player/MarioAnimator.cpp:38-40`) and drives actual BCK playback
through ActorRuntimeRegistry. The current camera snapshot only reads
jump/fast-movement predicates; grounding uses the original swimming status
query, not these animation queries. The restored water-mode virtual methods
must not be called on this unconstructed animation state. They are linked,
not runtime-validated or made safe by a false-return shortcut.

## Next real Xanime prerequisites and regression plan

The smallest actual player construction path starts at root
`XanimePlayer.cpp:14`: its original constructor needs a real J3DModel/model
data, an initialized XanimeResourceTable, original XanimeCore construction,
and `XanimeCore::initT`. The latter remains undecompiled in root (RMGK01
`0x8001ADE0`, size 0xE8); `XanimeCore::setBck` is also missing
(`0x80019460`, size 0x38) and needed by animation changes. No fake or partially
constructed Xanime object is sufficient for an `isRun` test.

For Mario, original `MarioAnimator::init` additionally requires its actual
resource holder and `marioAnimeTable`/single-to-quad tables, complete original
XanimeResourceTable initialization/sorting, lower and upper player ownership,
default-animation setup, original change/update operations, and the J3D joint
transform connection. The native actor runtime currently owns BCK resources
separately and has no compiled MR provider for the original J3DModel or
ResourceHolder getters. This resource/object ownership bridge must precede
removing the pre-existing animator nulls; the query import does not provide it.

Once those real objects exist, use the supplied disc's Mario animation archive
and original Mario animator tables to exercise both query branches: a current
authored group versus another authored group, and an original simple BCK
change versus the same/different motion resource. Check default/current group
identity, stopped animation state, and upper/lower MarioModule query behavior.
Repeated queries must not advance playback. The water-query check should use
the original `水泳ジェット` group and an unrelated current group, then the real
Mario target predicates. This requires original constructors and change
methods, not a byte buffer or manually populated XanimePlayer.

An earlier independent resource-lookup test can use the original ResTable and
ResFileInfo constructors/add methods in a test-only source slice, populated
with named resources borrowed from the actual disc archive. Test exact/missing
names, ASCII case equivalence, non-ASCII byte hashes, and returned resource
identity. That can validate the four restored ResTable lookups before the
full player exists; it does not establish end-to-end Xanime correctness.

No `Game/` algorithm, native target owner, runtime service, scheduling,
or build file was changed by this import. No compilation was run by the
source-import agent; the root task owns build and runtime validation.

Integration must retain one real target and bind its `mActor` before use.
The MR wrappers resolve the original MarioHolder, which must identify that
same live actor. Invoke target movement once per enabled camera phase:
original `CameraDirector.cpp:116-121` updates its target before its manager,
and `SceneExecutor.cpp:27,59` executes Camera before Player. The target reads
the previous completed player movement with the current published input.
Repeated reads must not rerun target movement and falsely suppress demo
movement through an unchanged timer.

The bound `_934` path also requires the real rush sensor/host and its Binder
for grounding queries. The original `getMapBaseMtx` requires Mario's `_46C`
triangle before it checks the triangle's sensor; do not eagerly sample unused
getters without their original state. The original constructor preserves its
unbound state, so dead/clipped early return cannot initialize a new target.
These original preconditions were preserved, not replaced with invented
target values.

`MarioAccess::getBaseMtx` is now available with the exact forced `_EA5`/`_EA8`
branch. Existing `MR::getPlayerBaseMtx` still belongs to PlayerUtilCompat and
the live player bridge at the time of this import; the integration owner
must preserve the actual forced-matrix behavior there. Wider player,
CameraDirector, and camera-selection parity remain separate work.
