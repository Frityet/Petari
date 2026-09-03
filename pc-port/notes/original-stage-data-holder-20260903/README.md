# Original StageDataHolder construction and layer loading

Root source recovery required by the actual CameraDirector constructor:
CameraRailHolder immediately queries the real StageDataHolder rail graph.
This checkpoint restores four missing methods and corrects one existing
iteration bug. It does not activate a native stage holder or camera.

| Method | Retail bytes | Original compiler score |
| --- | ---: | ---: |
| Constructor | 236 | 100% |
| initLayerJmpInfo | 480 | 99.75% |
| initPlacementInfoOrderedCommon | 332 | 100% |
| initPlacementInfoOrderedScenario | 312 | 100% |
| createLocalStageDataHolder | 320 | 99.875% |

All **1,680 bytes are exactly equal after verified original relocations**.
`verify-original.py` compares every instruction, branch, call and data
reference against RMGK01 DOL SHA1
25c5959534b3c21246c6c7e42021b916b41fb578. It also verifies all 17 layer-table
pointers and their actual strings. The empty-string reference uses r13 SDA,
with the actual retail base/target checked explicitly. No relocation is ignored.
`compiler-evidence.json` records the commands, source hashes and resolved refs.

The constructor obtains the real stage archive, retains the exact stage-name
pointer and authored zone ID, initializes original arrays/fields, clears the
24 child pointers, and initializes its placement matrix. Layer loading counts
both Placement and MapParts directories for each selected common/scenario bit,
allocates actual JMapInfo arrays and attaches resources in original order.
Child-holder construction reads every StageObjInfo row, resolves its real zone
through GalaxyStatusAccessor, constructs and initializes that actual child,
then computes its placement transform. Original fixed capacities are preserved.

The existing common-ordering method incorrectly incremented its child index
twice, skipping alternate child tables. Removing the extra increment produces
100% original code; no speculative ordering change is introduced. The new
scenario-ordering method preserves separate high/normal-priority lists and
sorts the existing planet list last.

`mPlacementMtx` is now its actual TPos3f type. The former raw Mtx declaration
required pointer casts and could not express the recovered original identity
call. The two SceneUtil accessors return its address with const_cast. Isolated
before/after original compilation verifies their two bodies, calcPlacementMtx,
and the generated StageDataHolder destructor: all 472 instruction bytes and
relocations remain unchanged (`verify-layout.py`, `layout-evidence.json`).

Root files: src/Game/Scene/StageDataHolder.cpp,
include/Game/Scene/StageDataHolder.hpp, src/Game/Util/SceneUtil.cpp.
`root-manifest.json` records their hashes. No native production edits, shared
builds, GPU sessions, commits or pushes were performed by this agent.

## Native construction frontier

The complete literal root source/header are staged in
build/original-stage-data-holder-20260903/staged. Native compilation is
**not yet complete**; native-compile.log and native-command.json record it.

* Original findPlacedStageDataHolder and updateDataAddress require actual raw
  BCSV header/row address ranges. Current native JMapInfo uses shared parsed
  DataCompat metadata and does not retain source address/stride/offset identity.
  Comparing DataCompat allocation addresses would give incorrect overlapping
  ranges and is not a valid adaptation. This needs a general retained JMap
  source-identity boundary and pointer-width arithmetic, with root instruction
  proof. It must also handle resource aliases and copies correctly.
* getStartJMapInfoIterFromStartDataIndex still explicitly reads JMapData. It can
  use the existing original getNumEntries accessor with original compiler
  verification, instead of assuming the host compatibility storage layout.
* The native JMap utility header lacks the existing checkJMapDataEntries
  declaration, and JKRArchive lacks getIdxResource, needed by original archive
  directory attachment. These are general API/provider gaps.
* PlacementInfoOrdered currently has only its constructor recovered. Its
  attach/sort/group/index methods are required by StageDataHolder::init and
  initAfterScenarioSelected; no native ordering substitute may fill this graph.

The actual parser owner fixture is separately complete in
original-camera-director-owner-20260903. Stage-holder source recovery and the
raw-KCL point/area fixture together still do not establish a complete scene,
CameraDirector, CollisionParts::init, Binder or Mario runtime.
