# Original actor animation and sound lifetime delta

This is a staged delta over `original-actor-model-retirement-20260903`, prepared
for the parent's atomic original actor/model activation. No native production
source, shared xmake target, ROM, or GPU run was changed by this subtask.

The complete patch is `lifetime-delta.patch`; `files.json` lists its nine files.
The corresponding files live in
`build/original-actor-animation-lifetime-20260903/staged/`.
Apply the delta to the reviewed retirement result, preserving any independent
parent scheduler/draw integration edits. `MarioAnimationEfx.cpp` is already
present in the native tree as an unselected root copy in some working states;
select the original source once rather than adding another provider.

## Original behavior and native ownership

`MarioAnimator::init` is byte-for-byte the root source body. The constructor
adds one ownership/allocation scope around that original call. The full root
`MarioAnimationEfx.cpp` is also copied unchanged, including the actual callback
functions, callback table, and Luigi animation swap table. No animation state,
frame control, BAS behavior, or effect callback was substituted.

The scope requires the actor's real retained ModelManagerOwner and the same
active scene ResourceHolderService allocation cohort. Host metadata is created
under JkrHostAllocationScope. Persistent JkrAllocationScope and J3dCommandScope
are entered only after that host escape ends, so the original init body actually
allocates in the scene heap. The scopes restore the previous heap-routing,
J3DSys, GD command, interrupt, and scheduler state on exit.

The ModelManagerOwner now accepts lifetime dependencies as shared ownership
metadata. This API does not perform Game algorithms. A dependency must never
retain the ModelManagerOwner itself. The constructor scope holds the owner only
until init finishes, so the resulting graph has no shared ownership cycle.

The captured dependency owns the completed MarioAnimator object, resource table,
resource HashSortTable and its four arrays, callback HashSortTable and its four
arrays, lower/upper XanimePlayer objects, and lower/upper XanimeCore objects.
The actual lower and upper cores have independent track arrays and share the
lower joint and transform arrays. Exact pointer equality prevents double deletes.
The resource table's zero-length simple-group array is also owned. Global group,
BCK, offset, auxiliary, and Luigi tables are borrowed and never deleted.
The players' lazily created simple groups are read from those captured owned
players at retirement, so allocation after init is also released. This owner
adds no runtime animation allocation algorithm.

The dependency retains the scene heap and real resource archive owner. It
survives actor registry removal when a draw executor retains the first material
prototype's ModelManagerOwner. On final model retirement, the manager's original
captured player is restored, attached animator graphs are released, and then the
original generic core/player/model are destroyed. The animator object is deleted
last within its graph because core transform offsets can reference its matrices.
Current implicit player/resource/animator destructors and the empty original
core destructor do not traverse the actor or release these child allocations.

Construction pointer slots are initialized to null before calling literal init.
Original assignment-after-new semantics let the unwind capture see only completed
child objects. Capture allocates nothing and does not throw. On failure it does
not own the unconstructed outer MarioAnimator (C++ releases that allocation),
restores the model's prior player, and restores a leaked original MutexHolder
recursion count. Completed child allocations remain retained until model owner
retirement. An allocation from a failed inner constructor that was never assigned
is storage in the retained scene cohort and retires with that heap; this package
does not invent partial Game destructors or claim an allocation-failure runtime
probe. Constructor failure handling was inspected in source, not executed.

## Sound construction

LiveActor::initSound retains the original two branches and actual
AudAnmSoundObject constructor calls. A native allocation scope selects the same
scene heap and the registry adopts the completed sound object. Model-less actors
retain the active scene heap directly. A model actor must use its existing model
cohort. Replacement retains the old cohort until its old captured sound object
has been destroyed. Registry member order destroys the sound object before its
heap owner, and the existing registry teardown happens before actor storage ends.

The original LiveActor::addToSoundObjHolder call and
MR::actorSoundMovement invocation are restored. They require the complete
`audio-animation-boundary-20260903` provider; object audio remains disabled there,
while actual BAS scheduling runs. No null-only sound shortcut is retained.
Do not add another MR::startBas provider beside OriginalActorSound.cpp.

## Verification and remaining integration

All six affected translation units compile with the active native compiler flags
and the new, audio, retirement, actual ModelManager, and original draw include
overlays. `compile-results.json` records every command. Warnings are the existing
LiveActor override annotations and material fog-width comparison. The six staged
objects contain no duplicate strong external definitions. Source checks confirm
literal MarioAnimator::init and the complete literal MarioAnimationEfx.cpp;
`validation.json` and `source-hashes.json` record this.

No integrated executable or live lifecycle was run here. Parent owns selecting
MarioAnimationEfx.cpp and MarioAnimatorLifetime.cpp, the final link frontier,
original sound/effect/wait-control source activation, original draw scheduling,
and the real scene construction/removal/reload gate. The source changes are
ready for that gate; compile success is not a gameplay or lifetime-runtime claim.

Recreate and compile from the repository root:

```
python3 pc-port/notes/original-actor-animation-lifetime-20260903/stage.py
python3 pc-port/notes/original-actor-animation-lifetime-20260903/compile.py
```
