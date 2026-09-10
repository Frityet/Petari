# Original save chunk encoding boundary — 2026-09-10

`SaveChunkEncoding.hpp/.cpp` adapts serialized bytes around the six actual original virtual serializers. It does not own or duplicate story, flag, value, galaxy, star-piece, or launch-star state. Other signatures pass through unchanged, including existing config chunks that already implement their Wii byte order.

The final isolated LLVM 23 ASan + UBSan fixture freshly compiles all ten current translation units through the production CP932 compiler wrapper, links, and runs successfully: **554 checks, exit 0**. This establishes byte conversion, format validation, actual PLAY/VLE1 behavior, and all 188 original flag identities through the actual sorted gameplay table. It does **not** establish a full six-owner save roundtrip, complete original GameSequenceProgress activation, or gameplay success. The parent owns those integration checks.

The earlier **179-check** run in `fixture-*.json` used the now-removed save-only hash scope. Those files are historical evidence, not proof of the active compiler boundary. The current result and source/tool/binary hashes are in `final-sanitized-*.json`.

## Integration contract

- The holder validates all recognized payloads in its first pass, before invoking any original deserializer. Malformed non-PLAY input returns `-1`; original `0` and `1` outcomes are preserved.
- `serialize_save_chunk` and `deserialize_save_chunk` call the original chunk virtual methods. Original JSU streams remain host-native: raw bytes and typed scalar operations are not globally made big-endian.
- Only declared scalar fields are swapped. Byte flags, tagged records and unknown schema fields stay opaque. Structural parsing determines widths and bounds; original consumers still choose the actual owner and apply values.
- Scratch vectors use short `HostAllocationScope`s. They end before the original virtual call, preserving the caller's Game heap routing. No adapter-owned gameplay object or registry is introduced.
- Ordinary original Game source literals compile to CP932 through `script/game_execution_charset.py`. `MR::getHashCode` always hashes the supplied raw bytes; `SaveChunkHashScope` and `save_chunk_hash_bytes` are removed. Explicit native-to-Game text boundaries convert known UTF-8 once. Scalar save conversion remains independent of text encoding.
- Output buffers retain the original serializer capacity contract. In particular, SPN1 and GALA contain original direct writes; this adapter validates their produced structure and caller result, but is not an independent preflight size calculator for arbitrary undersized output buffers. The explicit prevalidation guarantee applies to incoming payloads. PCE1's fixed 32-byte output is checked before its original direct write.

## Exact format handling

| Signature | Original owner | Converted scalars and validation |
|---|---|---|
| PLAY | `GameDataPlayerStatus` | Seven bytes: story byte, stock `u32`, saved lives `u16`. Serialize preserves short prefixes. Deserialize fills the original initialized suffix `{0,0,0,0,0,0,4}`, then invokes the original method with seven host-native bytes. Its lives reset/supply decisions remain original. Null/high-bit signed extents preserve the existing prefix/default contract. |
| FLG1 | `GameEventFlagStorage` | An even-length sequence of `u16` flag IDs. No translation between already-truncated hash IDs. |
| PCE1 | `StarPieceAlmsStorage` | Sixteen `u16` counters, minimum 32 bytes. Extra trailing bytes are untouched. |
| SPN1 | `SpinDriverPathStorage` | Byte galaxy count; galaxy hash/size `u16`; byte scenario count/padding; scenario size `u16`; byte driver tags. All nested extents and terminating tags are checked before original direct access. A driver needs a prior zone tag; interrupted drivers need their extra byte. The original 16-driver capacity and 100-step parser limit are bounded. Galaxy tail bytes are opaque. |
| VLE1 | `GameEventValueChecker` | Complete four-byte `u16` hash/value pairs. Partial pairs are malformed. Payload size is unchanged, preserving the original `size / 2` iteration count and recovered retained raw destinations at EOF. |
| GALA | `GameDataAllGalaxyStorage` | Record count, attribute count and record size are `u16`; each descriptor has `u16` hash/offset. The first matching descriptor selects the field, as in the original accessor. Galaxy ID and eight coin counters are `u16`; star/first-play flags remain bytes. Reordered descriptors and record offsets, unknown opaque attributes and absent optional attributes are accepted. Required ID, known widths, known-field nonoverlap and every record extent are checked. |

## Why CP932 must precede hashing

The earlier UTF-8 execution literals produced a real saved 15-bit collision at `0x389a`: `テレサマリオ初変身` and `CocoonExGalaxy`. Mapping already-truncated IDs cannot distinguish them. The final build instead emits original CP932 literal bytes before **any** original Game hashing, including sorted-table construction. The saved IDs are now the distinct literal values `0x278e` and `0x389a`.

The original `GameEventFlagTable::getIndexFromHashCode` recomputes each name linearly using raw hashing. `GameEventValueChecker::findIndexFromHashCode` likewise recomputes its 25 compiled original names. The actual `GameEventFlagTableInstance` also constructs its sorted keys from CP932. The final sanitizer fixture verifies uniqueness and successful original sorted lookup for all **188** actual entries, including both previously colliding names; no shadow table or lookup rewrite is used.

Host fixture literals remain UTF-8. Japanese arguments passed into original methods use explicit `resource::encode_cp932`, with independent literal CP932 byte assertions. The same raw hash still produces the UTF-8 value `0xba504313` when deliberately given host bytes and the original CP932 value `0x1097f8c3` when given Game bytes, before and after serialization. There is no ambient hash mode to restore or accidentally double-convert.
Evidence: `flag-hash-inventory.json` records every saved ID and both encodings; `flag-lookup-collisions.json` records the complete table's collision groups.

## Isolated test evidence

`SaveChunkEncodingProbe.cpp` directly links these current original classes:

- `GameDataPlayerStatus`: literal seven-byte golden, every output/input prefix, output canaries, null handling, stock/story values and original supply/lives reset.
- `GameEventValueChecker`: the actual 25-entry/100-byte serializer, Japanese `絵本既読章` ID `0xf8c3`, roundtrip, one record through two iterations, two records through four iterations, and unknown-then-known retaining error `1` after EOF.
- Original `GameEventFlagTable::getIndexFromHashCode` and `GameEventFlagTableInstance`: the two formerly colliding saved names resolve through both actual lookup paths; every one of the 188 original table entries has a unique 16-bit sorted key and resolves to its own original object. Raw production hashing is checked without any save-specific scope.

SPN1/GALA/PCE1 and the flat FLG1 byte vectors use a clearly marked **format-only byte source/sink**, not substitute gameplay owners. Their independently written Wii goldens exercise endian fields, truncations, malformed counts, field widths, overlapping known fields, absent optional fields, reordered schema/offsets, duplicate keys, two records and opaque unknown bytes. Bad inputs must be rejected before the source/sink's virtual load is entered.

Run `python3 notes/original-save-owner-20260910/chunk-encoding/verify-final-sanitized.py` from the repository root. It freshly builds the actual Clang helper, recompiles all ten current TUs through the final wrapper with ASan/UBSan, links only those isolated objects, and runs the fixture. It uses the earlier compile manifest solely as an include/ABI flag template, without reusing old objects or old hash semantics.

`final-sanitized-helper.json` and `final-sanitized-compile.json` record exact commands, current source hashes and exits. `final-sanitized-link.json` / `final-sanitized-run.json` and corresponding logs record link/runtime commands, compiler-tool hashes and the tested binary SHA. Sanitizers halt on the first address or undefined-behavior error; leak detection is explicitly disabled, so this is not a leak-freedom claim. The executable and objects are local evidence, not committed deliverables. No root Xmake command or production source edit was made by this fixture migration.

The VLE1 raw-read recovery and restored JSU implementation belong to the sibling agents; this test consumes their actual frozen sources. It does not invent EOF padding or change the original loop to `size / 4`.

## Source references

- `src/Game/System/GameDataPlayerStatus.cpp`: `serialize`, `deserialize`.
- `src/Game/System/GameEventFlagStorage.cpp`: stored-ID serialization/deserialization; `GameEventFlagTable.cpp`: `getIndexFromHashCode`, `initSortTable`, `findFlag`.
- `src/Game/System/StarPieceAlmsStorage.cpp`: the sixteen-counter payload.
- `src/Game/System/SpinDriverPathStorage.cpp`: `SpinDriverPathStorageOne`, `SpinDriverPathStorageScenario`, `SpinDriverPathStorageGalaxy`, outer serializer/deserializer.
- `src/Game/System/GameEventValueChecker.cpp`: hash/value payload, retained destinations and `/ 2` loop.
- `src/Game/System/GameDataGalaxyStorage.cpp`: record schema and consumers; `BinaryDataContentAccessor.cpp`: first-match descriptor lookup.
- `src/compat/JSUStreamCompat.cpp`: raw host-native stream semantics; `src/resource/TextEncoding.cpp`: CP932 conversion.
- Parent integration: `src/compat/BinaryDataChunkHolderCompat.cpp` and `src/compat/StorySequencePlatformCompat.cpp`.
