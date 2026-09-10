# Original PLAY fixture migration

The fixture now constructs `GameDataPlayerStatus` itself and invokes the explicit `serialize_save_chunk` / `deserialize_save_chunk` boundary. The whole original class remains the serialization and gameplay-state owner. A separate raw serializer/deserializer check verifies that JSU preserves native scalar bytes; this distinguishes the original source contract from the Wii wire conversion and catches an accidental second byte swap.

Preserved and expanded checks in `tests/OriginalPlayerStatusStorageTests.cpp`:

- Literal seven-byte Wii golden payload `2a 01 02 03 04 12 34`, signature `PLAY`, and header hash `0x27c90f`.
- Every output capacity from 0 through 9, exact written prefixes, return positions, and all surrounding canaries.
- Every input length from 0 through 9, including partial stock/life fields, the five-byte legacy form, and ignored trailing bytes. Truncated multi-byte fields retain the original initialized missing bytes in Wii order.
- Null pointers and extents outside the original signed stream range.
- High bits in all saved scalar fields (`ff fe dc ba 98 fe dc`), without converting load into a gameplay clamp.
- Actual saved-life supply threshold at 9/10, clearing the private supply field through the original method, fresh active lives remaining four, and original reset/upper/lower clamps.
- Actual `BinaryDataChunkHolder` dispatch to original source and destination objects, including a literal 16-byte container/header golden and the seven-byte PLAY body.

The previous fixture depended on the removed derived `PlayerStatusStorage` provider and native holder-state map: `GameDataSession` generations, `copy_holder_state`, `holder_state_count`, explicit map retirement, and scene-heap escape. Those are removed rather than retained as obsolete compatibility APIs. Parent-owned `OriginalSaveOwnerTests` is responsible for real six-chunk `GameDataHolder` / `UserFile` copy, selected-user ownership, and lifetime coverage. This fixture makes no full-owner or sequence-startup claim.

Validation at freeze:

- Isolated native LLVM 23 syntax check: exit 0; only pre-existing missing-override warnings from the forced `LiveActor.hpp` include. Exact command and source SHA are in `syntax.json`, diagnostics in `syntax.log`.
- `git diff --check -- tests/OriginalPlayerStatusStorageTests.cpp`: exit 0.
- Source SHA-256: `2a8626488e25b1cb8dfa7e548ff364a76114621e64df1e618544cbc9c7f318c9`.
- Official target `smg-pc-original-player-status-storage-tests` has been handed to the parent for build/runtime proof; no Xmake, link, or runtime was performed in this migration task.

No production, reference, build registration, or Git index changes were made.
