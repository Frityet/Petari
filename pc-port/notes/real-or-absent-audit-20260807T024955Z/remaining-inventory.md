# Remaining real-or-absent inventory

This inventory classifies behavior, not words. Entries stay open until the
retail source/runtime path replaces them or the unsupported feature is absent.
Retail defaults and degenerate-vector handling are not synthetic fallbacks.

## Critical sequence path

- The compatibility-owned `StagePlayerRuntime.*` Mario stand-in is removed.
  No proxy actor, input/motion/spin loop, or follow camera is spawned. The real
  `Game/Player/MarioActor` source closure must own those capabilities; until it
  does, Mario is absent and player-required placement is rejected.
- The title-to-file-select transition is honestly blocked at the retail
  `FileSelector::receiveOtherMsg(ACTMES_AUTORUSH_BEGIN, ...)` boundary because
  the real MarioActor auto-rush binder event is not installed. Do not restore
  the removed global auto-rush broadcast or add a title-specific state jump.
- The current strict picturebook route therefore stops on the starfield before
  an active FileSelect lifecycle. A registered but dead FileSelect layout is
  not route evidence; validators must require visible lifecycle state.
- The latest fresh title smoke exits before frame capture when the save service
  reaches the deliberately absent retail `GameDataHolder` binary closure. It
  also leaves an old invented `CFG1` config record behind. The next save sweep
  must remove that serializer and every partial/legacy host container path;
  persistence must use the retail `BinaryDataChunkHolder` format or be absent.
- Real Korean BTP resources parse, resolve through `ActorAnimCtrl.bcsv`, and
  evaluate against real model materials/textures. End-to-end BTP start and
  material-packet evidence remains blocked behind real title activation.
- `Game/Map/FileSelector.*` and `Game/Screen/MiiSelect.*` are still host-heavy
  rewrites. Restore their exact source boundaries after the real player event
  path is available. The hardcoded Mii hit rectangles are already removed.
- `PrologueLetter.*` and `ProloguePictureBook.*` are exact source, but the
  five-page picturebook still needs end-to-end proof through the retail demo
  lifecycle after FileSelect.

## Gameplay and scene state

- Coin/Purple Coin state, Dark Comet state, 100-coin Power Star declaration,
  and Purple Coin UI are explicitly unavailable until the real scene-owned
  `ScenePlayingResult`, event, and layout objects are present.
- Real Binder contact response and scheduler-consumed frustum clipping are
  implemented. Mirror areas, clip-area Binder filtering, DeathArea queries,
  floor-code lookup, and roof/ground pressure remain explicitly unavailable
  when their real owners/data are missing.
- All actor-shadow mutation APIs are explicitly unavailable until projection,
  collision, and draw behavior exist. Metadata-only shadow success was removed.
- Stage collision queries use only explicitly registered KCL owners. Missing
  collision ownership is an error, while a real but empty owner reports a real
  miss without fabricating output vectors.
- Stage gravity is scene-owned and combines registered real gravity objects by
  retail priority/type/host/activation rules. Implemented point, parallel,
  sphere, box, and cylinder formulas use the decompiled behavior; unsupported
  gravity shapes and a missing gravity owner are rejected.
- `Game/LiveActor/LiveActor.*` still embeds host storage and should be restored
  behind a scene/runtime-owned actor registry. The current registry remains a
  migration boundary, not the final Game source boundary.

## Directors and ownership

- Talk requests, area queries, group joins, and programmable-demo registration
  no longer report success without their real directors/managers. The actual
  TalkDirector and the programmable retail DemoDirector/executor lifecycle are
  still uncompiled. Demo ownership, active definition, starter, and puppetable
  control are scene-local; no process-global demo state remains. The scene
  demo-sheet clock is real for the Time/SubPart subset it owns, and wipe rows
  require a real scene-owned wipe service.
- `SceneObjHolder.*`, `MessageSensorHolder.*`, and `ActorSensorUtil.*` are exact
  source boundaries. Their supported runtime state is owned by the active scene;
  no process-global fallback SceneObj or message sensor remains.
- `FixedPosition.*` is exact source backed by real RARC/BCSV/joint lookup.
  Missing resources and joints are absent instead of using the actor base matrix.

## Source-boundary migrations

- `Game/Screen/SimpleLayout.*`, `Game/Util/CameraUtil.*`,
  `Game/Util/PlayerUtil.*`, `Game/Util/LiveActorUtil.cpp`,
  `Game/Util/MessageUtil.*`, `Game/Map/FileSelectFunc.*`,
  `Game/NPC/MiiFacePartsHolder.*`, and the sequence/prologue sources now use
  exact Game source with generalized compatibility implementations where needed.
- `LodCtrl.*`, `NPCActor.*`, `StarPiece.*`, `AreaObjUtil.*`,
  `GameDataFunction.*`, `GameDataHolder.*`, and `NameObjFactory.*` now use exact
  Game source boundaries. Exact sources whose full retail link closure is not
  yet present are excluded and compiled only through external compatibility
  wrappers; unsupported required capabilities reject construction or access.
- The external NameObj factory exposes only a verified compiled subset of the
  retail creator table. It does not infer same-name archives, blanket-ignore
  actor-bearing tables, or advertise actors whose mandatory runtime closure is
  absent.
- `LayoutActor.*`, `LayoutManager.hpp`, and `LayoutPaneCtrl.*` are now exact;
  the PC-only `LayoutManager.cpp` is absent. Resource, pane, animation, draw,
  and lifecycle state lives behind the external `layout/` bridge. Several
  adjacent FileSelect screen classes and `LayoutUtil.*` remain migration
  boundaries.

## Completed real-or-absent removals

- Missing player and camera state no longer produces origin/identity/default
  poses; camera-relative input and 3D drawing require real camera state.
- Missing BMG identifiers, RFL databases, localized names, and Mii face assets
  remain absent. No Mario/SMGPC Mii records, procedural faces, English names,
  or fake RFL success are manufactured.
- BCK frame passage uses the real J3D frame-controller semantics. BRK/BTP do
  not report stopped/playing without parsed animation data.
- Renderer approximation packet paths and placement-derived collision discovery
  were removed. Unsupported JPA shapes and unsupported placement actors are
  omitted/rejected rather than substituted.
- Stage data outside the real ZoneList, missing object-name rows, missing joints,
  and missing named layout panes remain absent.
- Forced transition keys/environment variables, title-specific rush broadcasts,
  global actor-name death teleports, and synthetic Gateway progression have
  been removed. Strict route validation now remains red at the first missing
  retail lifecycle instead of accepting registration or fabricated progress.
- Rumble uses the exact retail named pattern table and a real Aurora SDL/device
  actuator; unknown patterns and unavailable motors return `false`. Camera
  shake uses the retail seven strengths, 25-frame damped sine, and the exact
  `30 / screenWidth`, `30 / efbHeight` projection scaling. Effect deletion
  requires a registered effect keeper instead of succeeding as a no-op.

## Measured source boundary

- The 2026-08-07 audit compares each `pc-port/src/Game` C++ source/header to
  `src/Game` or `include/Game`: 154 of 308 comparable files are byte-identical
  (50.0%). The remaining 154 different files are the explicit migration
  inventory; no exactness claim is made for them.
- `LiveActor`, FileSelect UI, save-system wrappers, `LayoutUtil`, and several
  utility aggregates remain the largest host-heavy Game-side boundaries to
  restore.

## Confirmed retail behavior to retain

Do not remove the retail `MarioPosDummyModel`, camera-target dummy,
`MiiSelect::DummySelected`, `ShaMiiDummy`/`PicMiiDummy` panes,
`SE_SY_FILE_SEL_MORPH_DUMMY`, `RailPart::DUMMY`, or `dummyNoDrawWait` merely
because of their names. Retain retail vector-degeneracy defaults, JMap field
defaults, LightData's proven default-area selection, and the original
`FileUtil` fallback parameter. Host-injected SysConfig values, legacy save
readers, and byte-order alternatives are not covered by this exception: each
must be proved from retail behavior or removed.
