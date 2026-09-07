# Save-data owner and chunk closure inventory

Read-only production audit plus isolated reference/native compile probes. No save capability has been activated by this inventory. A separate first PLAY owner migration is drafted in notes/original-play-chunk-ownership-20260907 under parent coordination.

## Immediate conclusion

The complete original SaveDataHandleSequence.cpp is already mirrored byte-identically but excluded in Game/xmake.lua; SaveDataHandleSequenceCompat supplies unavailable operations. It compiles unchanged with only the missing original NANDErrorSequence header in an overlay. The complete original NANDErrorSequence compiles with four missing SDK NAND_CHECK constants (1,2,4,8 from revolution/nand.h:33). Its direct project provider gaps are requestGoWiiMenu(bool) and setResetOperationReturnToMenu, not a huge UI compile barrier.

The blocking owner is GameDataHolder serialization. Native selected-profile HolderState stores lives, supply, stock, story progress, sparse event maps and actual per-galaxy records, while the original six chunk pointers and BinaryDataChunkHolder are null. makeFileBinary and loadFromFileBinary explicitly throw. Original SaveDataHandleSequence initialization constructs two UserFiles and SaveDataHandler; handler construction immediately initializes nineteen save members through UserFile serialization. Consequently a sequence/UI import cannot complete original construction until the player/config chunk graph is real.

## Original six-chunk order and existing native state

| Chunk | Original owner | Native state to migrate | Specific prerequisites |
| --- | --- | --- | --- |
| PLAY | GameDataPlayerStatus | player_left, player_left_supply, stocked_star_piece_num, story_progress | Whole original source compiles; eleven methods and named vtable compare100%. Fixed payload is story u8, stock u32, saved lives u16. Deserialize resets active lives to4 and places saved lives into supply, with optional legacy absence. |
| FLG1 | GameEventFlagChecker -> GameEventFlagStorage -> MR::BitArray | event_flags map, derived predicates | Real BitArray and original flag-table singleton/index helpers; canOn needs standard C++ scopes around case locals. Flags excluded by mSaveFlag bit0 are not persisted. |
| PCE1 | StarPieceAlmsStorage | sixteen star_piece_alms u16 values | Whole methods compile; current reference header's declared destructor has no definition/retail symbol and leaves native vtable unresolved. Audit its artificial Dummy2/multiple-base layout before selecting the owner. Payload is raw32bytes of Wii u16 values. |
| SPN1 | SpinDriverPathStorage -> galaxy/scenario/one records | No current complete storage in HolderState | Actual ScenarioData catalog, full original MR vectors, JSU byte/seek API; raw nested headers and quantized path data need explicit byte-order handling. |
| VLE1 | GameEventValueChecker | event_values map, including MissNum and MissPointForLetter | Actual25-value table/defaults; JSU typed readU16/writeU16 and error/seek state missing in native minimal streams. Original decode loop behavior needs preservation, not a guessed cleaner rewrite. |
| GALA | GameDataAllGalaxyStorage -> GameDataSomeGalaxyStorage | current real per-galaxy objects in map | Full BinaryDataContentAccessor/HeaderSerializer, process ScenarioData ownership; original file attribute layout and u16 fields. Aggregate-only legacy imports do not establish individual star bits and must remain explicitly incomplete. |

GameDataHolder additionally creates ScenarioProgressTestRun and JMapInfo attached to original StoryEventBCSV. Its resource binding needs the actual process metadata instead of treating the reference empty static JMapInfo declaration as populated. Current GameDataRegistry contains verified tables used by native predicates; migration must preserve that data provenance and current selected/scene-start bindings.

## Fresh evidence

All nine reference chunk/holder TUs compile0 with the configured Wii compiler. Named compared symbols (section pseudo-symbols excluded):

- PLAY12/12exact; FlagChecker11/11exact; FlagStorage8/9exact,min99.18367; VLE1 11/12exact,min98.67347.
- PCE1 10/10exact; SPN1 38/38exact; ScenarioProgress1/1exact.
- GALA26/28exact,min97.5; GameDataHolder42/46exact,min89.09677 (onGalaxyScenarioFlagAlreadyVisited). Do not claim that lower-score method is validated equivalent.

Native overlay probes: PLAY, FlagStorage, PCE1, GALA and ScenarioProgress compile0. FlagChecker has switch-scope diagnostics; VLE1 and SPN1 need missing real JSU APIs; Holder has a signed clamp overload ambiguity. Native-external-inventory.json is a direct static-symbol inventory against current app/render/common/game archives plus successfully compiled candidates, not a full link/runtime proof. It excludes the removed stale Mario slice. SDK/system dependencies are not counted as project gaps. All candidate source and header files remain under notes.

## Remaining persistence/UI boundaries

- Original UserFile and ConfigDataHolder are mirrored unchanged. Config chunks and BinaryDataChunkHolder have existing byte-order-aware compatibility providers, but complete UserFile data serialization remains absent because PLAY/FLG1/PCE1/SPN1/VLE1/GALA are absent.
- Native SaveDataHandler is a divergent host implementation, including an extra constructor read and mSaveDataDirty field. Native NANDManager executes requests synchronously, truncates file paths, returns unconditional success for capacity checks and does not propagate host exceptions into original result state. Move these host boundaries outside Game when restoring the original owner, and preserve real request completion/result/callback behavior. A working filesystem primitive is not proof that original NANDManager semantics work.
- SaveDataService already persists/validates the complete big-endian GameData.bin container and translates the old native host-order container at its existing boundary. It cannot encode missing player chunks. Avoid writing both raw host bytes and already translated Wii bytes through the same conversion twice.
- Native SaveDataBannerCreator currently builds a zero-filled banner; original uses actual NANDInitBanner, system messages and retained texture data. It needs restoration before claiming the full save transaction is original.
- SysInfoWindow and IconAButton are mirrored whole; YesNoController diverges and requires original input/animation review. SaveIcon differs in constructor/layout naming and should be compared to reference. Original save UI is executed as children of the save sequence, not ordinary world movement; the process owner must update/draw it while SaveAfterGameOver has paused world movement.
- Native GameDataSession binds a holder directly with no owning UserFile. A complete save owner must unify current/backup UserFiles with these actual selected-profile bindings; copying aggregate telemetry into a separate serializer would lose original state.

## First cohesive cohort approved by parent: PLAY

Import whole original GameDataPlayerStatus source/header unchanged. Host selected-profile HolderState retains a concrete typed object and exposes its actual original mPlayerStatus pointer. Remove duplicated lives/supply/stock/story fields and delegate their reads/writes to that object. Copy/reset preserve destination object identity and copy the original private supply value through the normal class copy operation; typed owner retirement clears the borrowed pointer.

Keep JSU raw read/write as byte copies. A native final serialization adapter around original PLAY virtual methods converts only the known fixed payload scalar bytes using Aurora's endian helpers. This preserves Game source and its initialization/load/resupply algorithm; it does not infer types from arbitrary void pointers. Prove complete and truncated serialized payloads, legacy5byte loading, supply9/10 boundary, actual BinaryDataChunkHolder header/payload dispatch, selected-profile copy/reset isolation and scene/profile generations. Full holder serialization remains unavailable until all six chunks are complete.

After PLAY: implement the real JSU stream scalar/seek/error interfaces and VLE1 owner (needed by actual death/game-over counters), then FLG1, then GALA/SPN1/PCE1 with their metadata and raw-layout boundaries. Retire each old duplicated native store as its corresponding real chunk becomes sole owner. Only after all chunks are verified, restore GameDataHolder/UserFile/SaveDataHandler/NAND transaction, then original SaveDataHandleSequence/NANDErrorSequence and full Yes/No/reminder/error flows. Keep root scene transition policy separate from this reusable save-data pipeline.
