# Original ModelManager ownership and Game draw-buffer boundary

This is a staged compilation and source audit, not a production activation or a
rendering result. No Game algorithms, native owner APIs, shared build targets, or
root recoveries were changed by this task. The parent is implementing the actual
`ModelManager` owner and coordinates publication.

## Staged four-unit closure

`verify-native-draw.py` copies these root sources and headers byte-for-byte into
`build/original-model-manager-render-20260903/staged/Game/System/` and compiles
four independent objects with the current native Game compiler configuration:

- `DrawBuffer.cpp/.hpp`
- `DrawBufferExecuter.cpp/.hpp`
- `DrawBufferGroup.cpp/.hpp`
- `DrawBufferHolder.cpp/.hpp`

All four compile. `draw-native-evidence.json` contains source hashes, exact
commands, and exit codes; the four `*.compile.log` files contain diagnostics.
No decompilation or new draw algorithm was needed: all methods already have root
bodies. This does not establish a new retail instruction-match percentage.

The only missing compile boundary was `std::for_each_array`. The root MSL
`libs/MSL_C++/include/algorithm` defines this as a forward loop calling `f(first)`
for each element pointer, whereas ordinary `std::for_each` calls `f(*first)`.
The probe extracts that exact template into the staged
`MetrowerksDrawAlgorithm.hpp` and force-includes it. The eight Game files remain
unchanged. Publication can add this literal generic helper to the existing
native Metrowerks standard-library compatibility boundary.

Reproduce from the repository root:

```sh
python3 pc-port/notes/original-model-manager-render-20260903/verify-native-draw.py
```

The script also examines the four objects and current game/common/render static
libraries. `draw-symbol-evidence.json` distinguishes library presence from
correct source semantics. Excluding platform C/C++ runtime symbols, only these
four direct dependencies are missing from those current libraries:

| Symbol | Original provider |
| --- | --- |
| `MR::getJ3DModel(const LiveActor*)` | `src/Game/Util/ModelUtil.cpp:71` |
| `MR::getMaterial(J3DModel*, int)` | `src/Game/Util/ModelUtil.cpp:136` |
| `MR::getMaterialNum(J3DModel*)` | `src/Game/Util/ModelUtil.cpp:144` |
| `MR::getMaterialName(const J3DModelData*, int)` | `src/Game/Util/ModelUtil.cpp:148` |

The parent owns these literal accessor imports together with ModelManager's
other material helpers. Existing providers supply original J3D shape/packet
drawing, display-list calls, ShapePacketUserData, J3DSys, and light entrypoints.
Library presence alone does not clear the lighting issues below.

## The render boundary is the original Game draw buffer

Root `LiveActor::calcViewAndEntry` only calls `ModelManager::calcView`; it does not
call `J3DModel::entry`. Galaxy uses its own complete `DrawBuffer` family to draw
the actor's actual packets. Replacing the native actor renderer with a new pair
of SDK J3DDrawBuffers or per-actor `model.entry()` loops would miss that behavior.

The staged original code preserves these rules:

- `DrawBufferGroup::registerDrawBuffer` groups by original model resource name,
  preserves executer insertion order, and records the count before allocation.
- `DrawBuffer::initTable` coalesces material diff identities, uses the original
  opaque/translucent predicate, then sorts each partition by the original
  case-insensitive material name order. It preserves the material-to-drawer
  mapping after swaps. Do not reinterpret the misleading predicate name.
- `DrawBufferShapeDrawer` keeps the original packet order within light groups,
  emits a group light load only where required, and promotes the next packet's
  loader when removing the previous group's first packet.
- Drawing calls the original material packet display list, first shape
  pre-draw setup, original ShapePacketUserData list, each applicable light load,
  per-shape difference list, then `J3DShapePacket::drawFast`.
- Group/executer activation uses the original swap-removal behavior. Entry
  walks the active executers and calls each actor's virtual calc-view method.
  Opaque/translucent passes retain original J3DSys draw modes 3 and 4.

The authoritative category capacity/light/camera table already exists at
`src/Game/Scene/SceneNameObjListExecutor.cpp:289`; its construction at line 893
passes the complete table to `DrawBufferHolder`. The native scheduler should
use this original table and source category ordering instead of inventing
capacity or material classifications.

## Registration, lifetime, and phase ownership

The actual source has two initialization phases. `NameObjExecuteInfo::setConnectInfo`
registers the actor and records the returned executer index. Later
`SceneFunction::allocateDrawBufferActorList` calls the allocation method and
then `MR::initConnectting`, which activates requested actors. Native scene
construction needs the equivalent explicit completion point. Fixed-capacity
lists must not be silently rebuilt every frame or grown through changed Game
algorithms. Late registration needs a truthful scene lifecycle decision.

`SceneScheduler` should retain the returned index with each registered actor,
forward draw connection changes to the real holder, and invoke holder entry by
the active camera type exactly once. Current independent
`execute_calc_view_and_entry` actor iteration must be retired when holder entry
owns it. Camera-specific 2D, 3D, and mirror setup must precede each matching
entry/draw phase. Per-category drawing calls the original holder's opaque and
translucent functions in source order, rather than native per-actor rendering.

There is a crucial prototype lifetime: `DrawBufferExecuter` constructs its
`DrawBuffer` from the first registered actor's model. Each shape drawer stores
that model's `J3DMatPacket*`; removing the actor only removes its shape packets.
It does not replace the prototype model or material packets. Thus the scene
draw owner must retain the actual prototype model owner and its resource leases
until that executer is destroyed, even after actor deactivation/removal.
Retaining allocation bytes alone is insufficient if a destructor releases
external resources used by those packets.

All draw lists and owner records use the same explicit scene allocation cohort
as `ResourceHolderService`. An independent per-model cohort is unsafe:
`ModelManager::initMaterialAnm` can allocate holder-owned `mMaterialBuf` in the
current heap, and that holder can outlive an individual model. Both the model
and animation ResourceHolder leases remain retained; identical leases need not
be duplicated. No model-name-specific or actor-type-specific heap is needed.

Original pointer arrays mostly follow arena ownership. `AssignableArray`
destroys its own arrays but not pointed-to DrawBufferExecuters, DrawBuffers,
shape drawers, and PacketInfo allocations. A native scene owner must explicitly
describe those lifetimes; it must not claim that default destruction recursively
frees the graph. Such an owner must be noncopyable. It must deactivate actors
before discarding their shape packets/light controllers and retire queued GX
references before releasing the original models, holders, or shared cohort.

## Atomic actor migration boundary

The current `ActorRuntimeRegistry` keeps a private `LiveActorModel`, independent
BCK/BRK frame controllers, and animation names. `LiveActorModel` owns a
`J3dModelRenderer` whose private joint tree and XanimeCore provide a second set
of transform matrices and whose native material animation sampling bypasses
the real players. These cannot remain the Game actor's authoritative model
after publication of the actual ModelManager.

The smallest coherent change is one owner of the actual ModelManager graph,
retaining the shared service cohort and resource leases, with the actor's
`mModelManager` pointing at that exact instance. Root ModelManager already owns
the original model/Xanime player and all BTK/BRK/BTP/BPK/BVA players. Restore the
original LiveActor model initialization, animation update block, calc-animation
and calc-view bodies against that owner. Remove the scheduler's extra native
animation synchronization and route MR animation accessors to original methods.
The actual model's joint/matrix buffers must answer joint queries and supply
draw packets; do not copy them into a second renderer-controlled joint tree.

The native `MaterialCtrlCompat.cpp` fake global MaterialCtrl/Projmap definitions
must be removed atomically with the actual imported class and provider. Root
Projmap stores live `J3DTexMtxInfo*` and copies the original initial-effect matrix
baseline from ResourceHolder; it is not a native renderer projection override.
Archive/model name, animation, joint, bounding, LOD, shadow and effect callers
that currently obtain `LiveActorModel*` must all be migrated or explicitly
remain standalone renderer clients. In particular, changing only draw dispatch
would leave shadow/effect/LOD queries observing the old model.

Construction and post-load display-list work run under the retained original
JKR allocation scope and reusable `J3dCommandScope`; original constructors can
leave GDCurrentDL pointing into their local command-object storage. Native
host cache/registry allocations escape through the existing host scope.
J3dCommandScope does not restore J3DSys or J3DMtxCalc traversal globals. Any
additional reentrant owner scope must restore the real global state and model
binding, rather than run a separate calculation. Parent owns that model scope.

## Existing provider corrections required at publication

1. `MR::getModelResName` is currently implemented in
   `compat/LodCtrlRuntimeCompat.cpp:71` using the native archive name. Original
   `src/Game/Util/LiveActorUtil.cpp:816` returns
   `getModelResourceHolder(actor)->mModelResTable->getResName(0UL)`. This is the
   real grouping key and must be restored alongside ModelManager ownership.
2. `compat/NameObjExecuteCompat.cpp::findActorLightInfo` currently sets only the
   light type. Original `NameObjExecuteInfo::findLightInfo` forwards to the
   holder with both category and executer index. That call additionally stores
   the real DrawBuffer in ActorLightCtrl and configures group light loading.
3. Native `ActorLightCtrl.cpp` defines `resetLightSort` as a no-op. Original
   reset, area-light transition, and blend completion call
   `_8->resetLightSort(this)`. These calls must be restored once the buffer's
   lifetime and light registration are real; otherwise groups keep stale light
   loaders after transitions. No new sorting algorithm is needed.
4. Native `LightUtil::loadLight` has compatibility fallbacks and does not dispatch
   original coin light type 4. Root dispatches the real LightDirector's coin and
   player methods and uses its default area for other types. Full source
   lighting requires that director ownership, not just a linkable function.

Useful activation verification will use at least two original models sharing a
resource, material diff identities and deliberately different area-light
identities; remove the first registered actor, then draw the surviving actor.
Check original entry order, one update/calc per phase, animation frame controls,
material/player detach, prototype retention, light-group reordering, and full
resource/cohort retirement. Staged compilation here does not replace those
runtime tests.
