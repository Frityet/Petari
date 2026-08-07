# Remaining real-or-absent inventory

This inventory classifies behavior, not words. Entries stay open until the
real source/runtime path replaces them or the unsupported feature is removed.

## Gameplay and scene state

- `compat/StagePlayerRuntime.*` and its unconditional `StageHostScene` spawn
  implement a custom Mario, movement, and follow-camera system. Import the real
  player/camera closure or omit the player.
- `compat/PlayerUtilCompat.cpp` returns origin, identity axes, zero velocity,
  world-down gravity, and instant animation completion when no player/model is
  attached. Player queries must require real state.
- `GameActorPhysicsCompat.cpp` uses nerve-step modulo timing for BCK frame
  passage and process-global coin counts. Those must use the real animation
  controller and `ScenePlayingResult`, or remain unsupported.
- `Game/Util/ActorSensorUtil.cpp` accepts joint sensor names but does not apply
  joint transforms. Joint-bound sensors need the real joint/keeper path or
  must not be created.

## Camera and rendering

- `RuntimeContext`, `StageHostScene`, and `Game/Util/CameraUtil.cpp` construct a
  default/follow camera when no real camera exists. Camera-dependent rendering
  and queries must require a real pose.
- `J3dModelRenderer.cpp` still labels CPU/legacy TEV packet paths as
  approximate. Unsupported material packets should be omitted until their
  Dolphin-verified semantics are implemented.

Exact model/animation lookup and JPA shape omission were completed by this
audit and are no longer open.

## RFL, messages, and visible names

- `runtime/RflService.cpp` manufactures Mario/SMGPC Mii records when `RFL_DB`
  is absent; `runtime/rfl/RVLFaceLib.cpp` and `Game/NPC/MiiFacePartsHolder.cpp`
  also report success without a real RFL runtime. Missing data must report
  no-data/not-available.
- `Game/Map/FileSelector.cpp` and `Game/Screen/MiiSelect.cpp` contain hardcoded
  English names. Use the real localized `FileSelectFunc::copyMiiName` path and
  UTF-16 RFL names.
- `Game/Util/MessageUtil.cpp` and `Game/Util/LayoutUtil.cpp` show a missing BMG
  identifier as visible text. Missing messages must remain absent or fail.

## Completed in this sweep

Placement-driven collision discovery and English object-name substitution were
completed by this audit: collision now starts empty and accepts only explicit
KCL registrations, while missing `ObjNameTable` rows remain nullable.

## Directors and ownership

- `DemoUtilCompat.cpp`, `DemoCompat.cpp`, and `TalkCompat.cpp` can report
  successful programmable demos/talk without the retail directors. Preserve
  the picturebook by importing the real director machinery; no-director calls
  must eventually fail instead of activating a process-global substitute.
- `Game/Scene/SceneObjHolder.cpp` fabricates a process-global holder when scene
  ownership is missing. Require a scene-owned holder.
- `FixedPositionCompat.cpp` ignores requested resource data and uses the host
  base matrix with zero offsets. Import exact resource lookup or remove this
  constructor path.

## Source-boundary migrations

- `Game/Screen/SimpleLayout.*` embeds thousands of lines of host
  renderer/resource code; move state behind layout compatibility APIs and
  restore the tiny root source.
- `Game/LiveActor/LiveActor.*` embeds host model/camera/render storage; move it
  into actor runtime registries and restore the root class.
- `NPCActorCompat.cpp`, `StarPieceCompat.cpp`, and `LodCtrlCompat.cpp` are
  partial replacements for existing decompiled Game classes. Compile the real
  classes and keep only generalized external support.

## Confirmed non-violations

Do not remove the retail `MarioPosDummyModel`, camera-target dummy,
`MiiSelect::DummySelected`, `ShaMiiDummy`/`PicMiiDummy` panes,
`SE_SY_FILE_SEL_MORPH_DUMMY`, `RailPart::DUMMY`, or `dummyNoDrawWait` merely
because of their names. Also retain real vector-degeneracy defaults, JMap field
defaults, LightData's retail default-area selection, SysConfig defaults, and
the original `FileUtil` fallback parameter.
