# Original six-chunk GameDataHolder prerequisite — 2026-09-10

**No native GameDataHolder activation has occurred.** The native GameDataHolder facade, selected-user state, host hashes, build files and active demo were not modified. Work consists of a production read-only dependency audit, scratch native compile probes, and two specifically authorized reference recoveries. There was no Xmake, native link/runtime test, staging or commit by this task.

## Recovered reference defects

The current `decomp/AGENT_DECOMP_GUIDE.md` was read. Only `decomp/src/Game/System/GameDataHolder.cpp` and `decomp/include/Game/System/StarPieceAlmsStorage.hpp` were changed. `reference-recovery.patch` is the exact diff; unrelated WPadStick work was preserved. `diff-check.json` records a successful whitespace check.

| Method | Previous behavior | Recovered behavior | Fresh Wii objdiff |
| --- | --- | --- | --- |
| `GameDataHolder::isPassedStoryEvent` | Compared event threshold >= stored progress | Tests threshold <= stored progress | **100%**, 132/132 bytes; baseline 99.69697% was semantically wrong |
| `GameDataHolder::setGameEventValueForBit` | Computed mask & ~value, discarding unrelated bits | Clears the specified bit from the original value; the true branch ORs that bit into the original value | **92.06896%**, 116/116 bytes; baseline 91.89655%, 120 candidate bytes |

Retail evidence is `notes/gateway-audit-20260907/restoration/retail/asm/Game/System/GameDataHolder.s`: 803B3628–363C loads threshold and stored progress then uses `subfc threshold, stored` and the carry to form stored >= threshold. At 803B2FA8 the original `andc` computes value & ~mask; 803B2FB4 ORs the original value with the mask for true. The mask remains a full `s32` shift result, matching retail; truncating it prematurely to `u16` adds an instruction. The remaining bit-method differences are only `cmpwi` scheduling and swapped value/mask registers. It does not add or omit instructions or alter call targets/control flow.

Both full-TU baseline and recovered Wii compilations exit 0, as do both objdiff commands. Exact commands, source/object hashes and remaining instruction differences are in `baseline-proof.json`, `recovered-proof.json` and `function-scores.json`. These percentages are for the named methods, not the entire GameDataHolder unit. Neither corrected method was copied to the port or run there.

### StarPieceAlmsStorage header

The reference declared an undefined virtual destructor. The native object consequently referenced an undefined class vtable despite every actual method being present. Retail has no destructor symbol, and its 56-byte vtable at 805E00A8 contains the same five chunk methods twice, without destructor entries. Constructor stores at 803BB7E8/7EC establish the two existing vtable addresses; the existing Dummy2/base layout was retained.

The header correction removes the nonexistent destructor and places `deserialize` before `initializeData`, matching both retail vtable groups. Removing only the destructor left those last two declaration-ordered entries swapped (91.66667% vtable); correcting their order makes the **entire named vtable 100%**. All **nine named methods remain 100%**, and the 64-byte maximum-value table is also 100%. Wii compile, native object compile and objdiff all exit 0. The resulting native object **defines** `vtable for StarPieceAlmsStorage`, instead of leaving it undefined. No destructor body or new lifetime behavior was invented.

Evidence: `StarPieceAlmsStorage.{baseline,recovered}-proof.json`, `StarPieceAlmsStorage.recovered-scores.json`, and `StarPieceAlmsStorage.recovered-native.nm`. The source `.cpp` was untouched. Native GameDataHolder/chunk activation remains with the parent task.

## Exact owner/source cohort

Original `GameDataHolder` registers these six chunks in order in `BinaryDataChunkHolder(4096, 6)`:

| Order / signature | Actual owner | Current native storage and required source |
| --- | --- | --- |
| 1 / PLAY | `GameDataPlayerStatus` | Whole original source already compiles normally. HolderState currently owns a `PlayerStatusStorage` derived object that delegates the gameplay methods and adapts the fixed serialized payload. Whole GameDataHolder will instantiate the original base class directly, so that endian adaptation must move to the general serialization boundary before switching constructors. |
| 2 / FLG1 | `GameEventFlagChecker` -> `GameEventFlagStorage` -> `MR::BitArray` | Current sparse flag map and partial duplicated predicates must be replaced by whole `GameEventFlagChecker.cpp`, `GameEventFlagStorage.cpp`, and `Util/BitArray.cpp`, plus their exact headers. The original flag-table singleton and table methods are already present. |
| 3 / PCE1 | `StarPieceAlmsStorage` | Current sixteen-element donation array moves to the real original storage. Add whole source/header with the header recovery above. |
| 4 / SPN1 | `SpinDriverPathStorage` -> galaxy/scenario/one records | No complete launch-path state exists in current HolderState. Add whole source/header; its constructor enumerates the actual ScenarioData catalog. |
| 5 / VLE1 | `GameEventValueChecker` | Current sparse value map moves to the real original 25-value array/default table. Add whole source/header; FindingLuigiEventScheduler.hpp is a declaration dependency for the existing STATE_NULL constant, not a requirement to construct mail services here. |
| 6 / GALA | `GameDataAllGalaxyStorage` -> `GameDataSomeGalaxyStorage` | Current map already stores genuine per-galaxy records. Replace `GameDataGalaxyStorageCompat.cpp` slices with whole original GameDataGalaxyStorage.cpp and enable whole BinaryDataContentAccessor.cpp. The original constructor filters the actual ScenarioData catalog to galaxies with Power Stars. |

The holder also constructs `ScenarioProgressTestRun` (whole source is only its actual constructor) and a JMapInfo for StoryEventBCSV. `GameDataConst.cpp` should be activated whole for its flag/galaxy dependency methods; its two methods currently in GameDataGalaxyStorageCompat and its unavailable `getIncludedGrandGalaxyId` in StorySequencePlatformCompat must lose their competing definitions. Existing GalaxyIDBCSV data and registration already support its JMap attachments.

Thus the direct source set is: GameDataHolder, GameDataPlayerStatus, GameEventFlagChecker, GameEventFlagStorage, GameEventValueChecker, GameDataGalaxyStorage, SpinDriverPathStorage, StarPieceAlmsStorage, ScenarioProgressTestRun, GameDataConst, BitArray and BinaryDataContentAccessor. Keep a single provider per method. This inventory does not certify unrelated methods' behavior solely because their source exists.

The process ScenarioCatalogOwnership must be initialized **before** the original holder: GALA and SPN1 constructors enumerate it eagerly, unlike today's lazy native galaxy map. It must outlive their borrowed galaxy-name pointers. Original GameEventFlagTableInstance also owns a process-lifetime sort table. Selected current/backup UserFiles and their chunks need a real process allocation domain and typed child retirement; these non-NameObj children are not all covered by the Scene NameObj ownership mechanism. BinaryDataChunkBase has no virtual destructor, and shallow copying whole holders or deleting chunks through that base would be incorrect.

## Fresh native compile inventory

`compile.json` records twelve independent unchanged-reference source probes using current native SDK headers plus exact missing Game headers copied under `include/`. The initial probes precede the two authorized reference recoveries; the saved baseline and header snapshots preserve their inputs.

- **Exit 0:** GameDataPlayerStatus, GameEventFlagStorage, GameDataGalaxyStorage, StarPieceAlmsStorage, ScenarioProgressTestRun, GameDataConst and BitArray.
- **GameDataHolder:** only ambiguous `MR::clamp(value + points, 0l, 20l)` caused by 64-bit host `long`. Resolve at the general overload/SDK compile boundary or a reference-proven type correction; no unrelated holder body was changed in this task.
- **GameEventFlagChecker:** four switch cases declare locals without scopes, so later case labels cross initialization. Standard C++ braces are the required compile correction; no copied predicate implementation is necessary.
- **GameEventValueChecker:** missing JSU `writeU16`, `readU16`, `mState`, `getState` and JSUIosBase error constants.
- **SpinDriverPathStorage:** missing JSU `readU8`, `writeU8`, `seek` and seek-origin enum.
- **BinaryDataContentAccessor:** missing `JSystem/JSupport/JSUInputStream.hpp`; its full source additionally uses stream position/seek/skip and typed reads.

Except for the corrected StarPiece header/vtable, these are compile/dependency observations rather than activated providers. No missing body was newly invented. The existing original JSUInputStream.cpp, JSUOutputStream.cpp, JSUMemoryStream.cpp and seven JSupport headers provide the source contracts for general stream expansion. Their memory source narrows pointers through `int`; preserve native width in the compatibility implementation. The reference JSUIosBase header represents bitmask state as `bool`, so its byte/error contract needs checking before blindly importing that header.

## File bytes and general compatibility

The current two native JSUMemoryStream headers intentionally provide **raw byte copies** and partial-copy position advancement. Changing arbitrary `read(void*, size)` or `write(const void*, size)` to byte-swap by size is invalid: those functions cannot know whether bytes represent scalars, arrays or already serialized data. Add the real typed/seek/error APIs, keep raw copies raw, and make the serialized-format boundary explicit.

| Format | Retail payload boundary |
| --- | --- |
| Chunk container | Four bytes `{version=1, count, 0, 0}`; each chunk has BE u32 signature/hash/total-size then payload. Current BinaryDataChunkHolderCompat already validates and writes these BE headers and invokes actual virtual chunk methods. It is not a throw-only serializer. |
| PLAY | Story u8, stock BE u32, saved lives BE u16 (7 bytes). Original load initializes defaults, assigns saved lives to supply and resets active lives to 4. Preserve partial/legacy behavior already tested by OriginalPlayerStatusStorageTests. An in-memory owner migration must copy the actual status value, not save/reload it, since save/reload changes lives/supply by design. |
| FLG1 | BE u16 records: hash(name)&0x7fff plus stored-bit at 0x8000. Derived flags (`mSaveFlag & 1`) are omitted. Original raw stream writes are native order today. |
| PCE1 | Sixteen BE u16 donation values copied as raw 32 bytes. Typed JSU helpers alone cannot fix this payload. |
| SPN1 | Leading galaxy count u8; nested galaxy headers contain BE u16 name hash/block size, scenario count u8; scenario headers contain BE u16 block size. Zone/driver/range tags are bytes. Direct cast/dereference fields may be unaligned and require explicit buffer adaptation. |
| VLE1 | BE u16 name-hash/value pairs. The reference deserialize loop uses `size / 2` while reading two u16 per iteration; **retail 803B57E0 also divides by 2**, so do not replace it with a guessed `/4` cleanup. Real JSU partial-read/error behavior matters here. |
| GALA | BE u16 galaxy count; content-attribute header with BE u16 count/data-size/hash/offset fields; each record has BE u16 galaxy hash, ownership/visited bytes and eight BE u16 coin values. Original methods access several fields directly as u16 pointers. |

Whole original chunk owners can stay responsible for game state and serialization algorithms while a generalized, explicitly typed save-format boundary adapts raw native payloads and validated Wii bytes. This must cover direct-memory formats as well as typed stream calls. It cannot be replaced by serializing the current host telemetry into a new format. When the original constructor creates GameDataPlayerStatus itself, preserve the existing PLAY behavior at that boundary and retire the derived stand-in; do not leave two statuses or patch the original constructor to select a host gameplay owner.

`aurora/endian.hpp` already supplies endian-safe field reads/writes. Existing BinaryDataChunkHolderCompat, SysConfigFileCompat and ConfigDataMiscCompat have BE-aware serialization. SaveDataService in RuntimeServices.cpp:3926–3963 translates the current legacy host-order **outer GameData.bin container** to/from the persisted BE container. It is not a six-chunk codec. Consolidate this translation when restoring original SaveDataHandler so already-converted bytes are not converted twice; retain current bounds/checksum verification and member identity checks.

### CP932 wire IDs

Endianness is insufficient. Original FLG1/VLE1 serializers hash Japanese names compiled as SJIS/CP932. Current MR::getHashCode in StorySequencePlatformCompat hashes current UTF-8 bytes. `wire-encoding-examples.json` records the differing IDs using that exact hash recurrence. An ASCII example remains equal. Save-format ID handling must use the retail encoding and preserve original lookup/collision semantics; do not change the global host hash behavior or silently write UTF-8-based IDs into a Wii file. `resource::encode_cp932`/`decode_cp932` already exist for explicit encoding boundaries. No hash code was modified.

## StoryEventBCSV is actual program data

Current reference declares `const static JMapInfo StoryEventBCSV`, then attaches its address. That is an empty C++ object, not the original packed table. The retail symbol is the **480-byte raw BCSV at 8053DC20**. This task extracted the exact `.4byte` data into `StoryEventBCSV.bin`, decoded all 14 CP932 rows, and verified exact equality with the present native GameDataRegistry name/progress rows (`story-table-proof.json`). The resource is not a stage archive; it is embedded original program data.

The correct future recovery is the actual data declaration/definition in decomp, then native mirror, following the existing GalaxyIDBCSV pattern. Register its complete immutable extent through `JMapResource`/`EmbeddedGameTables` before holder construction. Native JMapInfo::attach already requires such retained bounds and shares the decoded table lifetime. Do not special-case an empty JMapInfo address or construct a guessed story table inside GameDataHolder.

## Lossless selected-user migration

Current GameDataSession owns a holder with a null UserFile pointer and binds both current and scene-start views to that same holder. Its constructor explicitly establishes demo story progress 5. Original full save ownership instead supplies distinct current and backup UserFiles; story/selected-file initialization must come from the actual chosen profile/sequence, not an implicit reset to the demo default during migration.

Preserve these current fields into the corresponding actual chunks exactly once, then retire the parallel HolderState:

- Copy the real GameDataPlayerStatus value, including private supply, stock, active lives and story progress; preserve destination identity and do not use the PLAY load protocol for an in-memory transfer.
- Transfer stored flags to GameEventFlagStorage by actual table identity; preserve true and false states. Do not replay `tryOn`, since permission predicates may reject a legitimately saved flag. Derived flags remain derived.
- Transfer all explicit event values to GameEventValueChecker after its original defaults initialize; this preserves MissNum, MissPointForLetter, best times, explanation/read bits, Luigi/comet values and other saved counters.
- Transfer all sixteen donation values to PCE1 and each current GALA record's star bits, visited bits and eight coin values. Rebind name pointers to the process catalog rather than retaining map-key pointers that will be destroyed.
- Current code has no complete SPN1 state to migrate; initialize the real empty chunk only for a profile known to be fresh. Existing Wii profiles must load their actual SPN1 payload rather than assuming there is no launch-path progress.
- A positive aggregate star count imported without per-galaxy bits cannot be expanded losslessly. Current code explicitly marks such data incomplete; require actual save membership data or a user-requested reset, and never invent individual ownership bits. The aggregate setter/import helpers currently have no production call sites outside their definitions; their uses are tests, which makes retiring that alternative easier.
- The holder name/selected slot/player identity and owning UserFile configuration (including Mario/Luigi completion flags and Mii/name data) must belong to the actual current/backup user owner. Holder chunks alone cannot synthesize missing UserFile configuration.

Current copy/reset tests also deliberately permit a native copy to outlive ScenarioCatalogOwnership, because map keys own the names. Original GALA/SPN1 borrow the process catalog. Migrate those fixtures to the real process-lifetime contract, or retain that resource owner explicitly; do not keep the old copied map merely to preserve an obsolete fixture lifetime.

## Concrete implementation order and proof

1. Complete general JSU stream interfaces and a coherent serialized-byte/ID boundary; preserve existing PLAY partial-load proof and add typed/seek/error cases needed by VLE1/SPN1. Restore packed StoryEventBCSV data and bounded process registration.
2. Import the whole chunk/checker/BitArray/GameDataConst sources with the compile-only scopes/type adaptations, apply the two proven holder fixes and recovered PCE header, and establish actual process catalog/UserFile/chunk ownership. Remove each competing provider and the parallel HolderState in the same integration.
3. Migrate selected/current/backup bindings and test the actual six-chunk order, source-to-copy identity/isolation, complete Wii bytes, original reset/load effects, all story thresholds, flag bit preservation, authored scenario/coin/path records and process teardown. Use retail data for unknown-field/hash behavior; do not mirror a new serializer implementation in its tests.
4. Only then replace the divergent original save-handler/NAND/save-sequence facade and claim initialized sequence readiness. The preceding save-sequence audit describes that next owner cohort.

Existing relevant fixtures: OriginalPlayerStatusStorageTests, GameDataStarStorageTests, GameDataRealOrAbsentTests, SaveDataCoreRealOrAbsentTests, and the live GatewaySpinCheckpoint story assertions. Their absence/partial-owner expectations must be updated around actual readiness without turning unknown data into success.

`source-manifest.json` captures current inspected sources and the exact two reference edits. Large objects/objdiff dumps and scratch copied headers remain local supporting artifacts; the compact JSON proofs, this README, extracted table proof and exact patch are sufficient review notes for the checkpoint.
