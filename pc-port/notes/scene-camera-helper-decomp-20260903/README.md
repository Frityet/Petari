# Scene camera helper recovery

Recovered four missing root `SceneUtil.cpp` helpers from the supplied RMGK01
rev0 executable and corrected two narrow `StageDataHolder` issues encountered
along their dependency path. No native runtime, PC mirror, or xmake files were
changed by this task.

This root provider work prepares the full original `CameraDirector` startup
closure. It adds no native scene-specific substitution or camera-selection path.

## Recovered behavior

| Function | RMGK01 address | Size | Original behavior |
| --- | --- | ---: | --- |
| `MR::getCurrentStartZoneId` | `0x803f75a0` | `0x24` | Delegates to the active `StageDataHolder` |
| `MR::getCurrentStartCameraId` | `0x803f7a88` | `0x24` | Delegates to the active `StageDataHolder` |
| `MR::getStartCameraIdInfoFromStartDataIndex` | `0x803f7aac` | `0x44` | Forwards the output and index unchanged to the active holder |
| `MR::getStageCameraData` | `0x803f7c14` | `0x8c` | Checks placement, selects that zone's holder, returns its camera archive resource and size |

The first three functions call `MR::getStageDataHolder` at `0x80348b28`, then
the matching already-decompiled member at `0x803475b0`, `0x803475f4`, or
`0x80347640`. They do not reconstruct start IDs, inspect a synthetic camera
state, or substitute the root zone.

`getStageCameraData` has an explicit absent-zone branch: when `isPlacedZone`
returns false, it writes `nullptr` to the data output and zero to the size output.
Otherwise it obtains the holder through the **nonconst**
`getStageDataHolderFromZoneId` overload, calls
`getStageArchiveResource("CameraParam.bcam")`, and passes that returned pointer
to `getStageArchiveResourceSize`. The exact resource string is present at
`0x805e1fcb`. There is no archive fallback or separate resource-null branch;
the resource-size result is not rewritten to zero.

The existing `SceneUtil.hpp` declarations were correct and required no change.

## Related root corrections

`StageDataHolder::isPlacedZone`, at `0x80347b0c` with size `0x50`, returns true
for zone zero or an **equal** zone ID among immediate child holders. The existing
root implementation used `!=`. The retail comparison at `0x80347b3c` and branch
at `0x80347b40` explicitly skip the true return on inequality. Changing only
that comparison produces an exact instruction match.

The mutable `getStageDataHolderFromZoneId(int)` overload now returns
`StageDataHolder*`; its const overload still returns `const StageDataHolder*`.
The mutable overload forwards through the const implementation and casts the
result back to mutable. This permits the original helper's nonconst archive
calls without a cast at every consumer. Return-pointer const qualification is
not encoded in the binary, so this is a declaration/const-correctness repair,
not a claim that machine code alone recovers the qualifier. Its implementation
remains the exact one-instruction tail call at `0x80347b08` to `0x80347ac0`.

The earlier stage-zone-matrix task had already repaired this wrapper's accidental
self-recursion by casting `this` to const. This task preserves that forwarding
and adds the appropriate mutable return type.

## Verification

The verified DOL SHA-1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`.
Both full root translation units compile with the original GC/3.0a3 compiler
and Game flags from `configure.py`, with no diagnostics. The included verifier
resolves branch targets from `config/RMGK01/symbols.txt`, checks the actual
resource string for both address relocations, and compares every resulting
instruction against the DOL.

Result: **91/91 instructions match exactly across all six functions**, after
17 verified relocations. This claim concerns those six functions, not the
otherwise incomplete translation units.

With the existing local compiler, wibo, and extracted DOL available:

```sh
python3 pc-port/notes/scene-camera-helper-decomp-20260903/verify-objects.py --compile
```

Compiler commands, object files, and disassembly are stored under ignored
`build/compat-scene-camera-helpers/`. No binary assets are committed.
`git diff --check` passed for the three changed root files.

## PC provider requirements

PC already declares these APIs in `Game/Util/SceneUtil.hpp`, but its excluded
`Game/Util/SceneUtil.cpp` still contains the four placeholders. The file is
explicitly removed by `Game/xmake.lua`; no automatic mirror or activation was
performed. PC has no `Game/Scene/StageDataHolder.hpp/.cpp` pair at this point.
The existing native `StageZoneMatrixRegistry` does use equality and searches
immediate children in occurrence order, so it does not contain the corrected
`isPlacedZone` inequality bug.

An exact original-shaped provider closure requires:

- An active stage holder owner, consistent with
  `MR::getSceneObj<StageDataHolder>(SceneObj_StageDataHolder)`.
- The selected Mario start row and its actual owning zone. Existing root
  `getCurrentStartCameraId` reads `Camera_id` and returns `-1` if the field read
  fails. It does not use the currently active event or gameplay camera.
- Indexed start-row traversal in the original order: local start tables, then
  each child holder recursively in occurrence order. The member helper calls
  `JMapIdInfo::initialize(cameraID, iter)`, retaining the row's placed-zone
  identity. It assumes a valid row with `Camera_id`; it does not contain the
  current-start helper's missing-field `-1` fallback.
- Placed-zone membership and selected-holder archive ownership, plus stable
  `CameraParam.bcam` storage and the real archive resource-size behavior.

`StageAuthoredData` already retains authored rows, holder occurrences, and a
selected start. Those are appropriate inputs for a compatibility owner, but
they do not themselves implement these original methods. One further root
dependency, `StageDataHolder::makeCurrentMarioJMapInfoIter` (`0x80347d84`, size
`0xd4`), is still absent from the root source and was not guessed or recovered
in this bounded task.
