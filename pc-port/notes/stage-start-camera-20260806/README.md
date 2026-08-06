# Generalized stage-start camera and stage-environment verification

This slice keeps the decompiled `Game/` surface source-close and implements the
host-specific resource traversal, camera math, and runtime priority in the PC
compatibility/runtime layers. It contains no galaxy-name camera special cases.

## Implemented behavior

- Stage-zone transforms are rigid row-major 3x4 matrices, recursively composed
  for child zones. Placement points and orientation vectors use that matrix;
  zone scale is not incorrectly applied to original game placement data.
- `StartInfo` selection is driven by scenario layer, zone ID, archive order,
  and `MarioNo`/start ID. Camera ID and BCAM are loaded from the selected
  zone's own archive, including child zones.
- `CAM_TYPE_XZ_PARA` implements the original distance clamp, watch offset,
  vertical interpolation flag, zone-vector transform, roll, FOV semantics, and
  pre-`0x30016` migration of `num1`.
- The initial target/up basis comes from the transformed StartInfo orientation.
  The absence of a camera gravity query is explicitly traced as
  `fallback=transformed_start_orientation_up`.
- Runtime priority is free camera, then active programmable camera, then the
  persistent base-game/start camera.
- Stage request plumbing carries start ID and start-zone ID without changing
  the title, file-select, or picturebook route.
- `StageHostScene` activates the generalized placement-backed collision and
  gravity services for normal stages and clears them for explicit debug roots.

## Native and real-resource verification

Commands and results are preserved in [test-results.txt](test-results.txt).

The focused tests cover:

- synthetic nested rigid-zone composition and exact StartInfo selection;
- old/new BCAM migration, `s:%04x` key formatting, XZ_PARA eye/watch/up/roll,
  and FOV flags;
- base-game versus programmable-camera priority;
- real Korean-disc HeavensDoor root StartInfo (`MarioNo=0`, camera 78,
  `s:004e`, XZ_PARA);
- real child-zone StartInfo in MysteriousZone (zone 5, camera 999,
  `s:03e7`) loaded from the child archive.

The regular decomp is configured for `RMGK02`; `ninja` completes cleanly.

## Full route evidence

The aggregate [route manifest](route-smoke/manifest.json) passed all four
real-disc checkpoints:

| Checkpoint | Frame | Non-black | Render packets | Capture |
| --- | ---: | ---: | ---: | --- |
| Title | 90 | 1.0000 | 22 | [PNG](route-smoke/title/title-frame-90.png) |
| File select | 1900 | 0.999831 | 27 | [PNG](route-smoke/file_select/file_select-frame-1900.png) |
| Picturebook | 7600 | 1.0000 | 13 | [PNG](route-smoke/picturebook/picturebook-frame-7600.png) |
| Gateway handoff | 10350 | 0.980570 | 573 | [PNG](route-smoke/gateway_handoff/gateway_handoff-frame-10350.png) |

The gateway [application log](route-smoke/gateway_handoff/gateway_handoff-app.log)
records the selected HeavensDoor camera as camera 78 / `s:004e` /
`CAM_TYPE_XZ_PARA`, with the calculated eye, watch, rolled up vector, and 45
degree FOV. The same log records 61 collision meshes / 22,416 triangles and
seven supported gravity placements with no unsupported gravity type.

The [gateway trace](route-smoke/gateway_handoff/gateway_handoff-frame-10350.trace.sqlite),
[trace validator log](route-smoke/gateway_handoff/gateway_handoff-trace-validator.log),
and [placement report](route-smoke/gateway_handoff/gateway_handoff-placement-report.md)
are retained alongside the screenshot.

## Files in this camera slice

- `src/camera/CameraParam.cpp`
- `src/camera/StageStartCamera.cpp`
- `src/camera/StageStartCamera.hpp`
- `src/runtime/ParityTrace.cpp`
- `src/runtime/RuntimeContext.cpp`
- `src/runtime/RuntimeServices.cpp`
- `src/runtime/RuntimeServices.hpp`
- `src/scene/GameSystemSceneControllerService.cpp`
- `src/scene/GameSystemSceneControllerService.hpp`
- `src/scene/SequenceBootService.cpp`
- `src/scene/StageHostScene.cpp`
- `src/scene/StageHostScene.hpp`
- `src/scene/StageHostService.hpp`
- `src/scene/StagePlacementResolver.cpp`
- `src/scene/StagePlacementResolver.hpp`
- `src/scene/StorySequenceService.cpp`
- `src/Game/System/StorySequenceExecutor.cpp`
- `src/Game/System/StorySequenceExecutor.hpp`
- `tests/StageStartCameraTests.cpp`
- `tests/xmake.lua`

`StageHostScene` also contains the lifecycle integration for the separately
implemented `StageCollisionService` and `StageGravityService`.
