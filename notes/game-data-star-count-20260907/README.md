# Original galaxy Power Star storage — 2026-09-07

`GalaxyStatusAccessor::getPowerStarNumOwned` could not link because its original
GameDataFunction provider was missing. The native holder previously retained
only a single total; positive per-galaxy queries rejected incomplete data, and
the story boundary attempted to map every star through the special-star table.
That table cannot represent ordinary star ownership.

The original `GameDataGalaxyStorage.hpp` is imported byte-identically from
`decomp/include`. Ten original in-memory `GameDataSomeGalaxyStorage` and
`GameDataSomeScenarioAccessor` methods are copied without body edits into
`compat/GameDataGalaxyStorageCompat.cpp`. Two original GameDataConst methods
replace the unsupported Grand Star classifier using the actual special-star
event table. The native ownership layer supplies stable per-holder storage and
galaxy names, initialized from every star-bearing entry in the actual published
scenario catalog. Its allocations remain outside scene heaps.

The original count loops over ownership bits 1 through the catalog's
`getPowerStarNum`; it does not exclude hidden stars or count Grand Stars twice.
Bits outside that authored count remain representable in the original eight-bit
record and do not inflate totals. Host entry points reject indices outside 1–8
before invoking the original shift operations. Unknown galaxies and missing
catalog ownership are explicit failures.

GameDataFunction queries and awards now route to the actual selected holder.
Original scenario accessors update the same records used for per-galaxy and
aggregate counts; duplicate awards are idempotent. Grand Star event predicates
read those ownership bits. Holder copies deeply own their records/names, and
reset clears original records in place so retained scenario accessors remain
valid. Existing scoped selected-file bindings are preserved.

`set_holder_save_counts` still accepts aggregate-only input, preserves that
total, and explicitly rejects per-galaxy queries or writes when a positive total
has no ownership bits. Zero total proves all bits clear. The whole-file save
serializer/deserializer remain unavailable; this change does not invent a GALA
file format or claim persistent save loading. Full `GameDataAllGalaxyStorage`
construction/serialization is not activated because its BinaryDataContent
serialization dependency remains excluded from the native build.

Validation target: `smg-pc-game-data-star-storage-tests` with `SMGPC_REAL_DISC`.
It checks every authored galaxy, hidden/Grand Star behavior, original accessor
coin/visit fields, selected-file isolation, deep copy/reset, host allocation
under a retiring Game heap, retained names after catalog destruction, and
aggregate-only rejection. The LLVM 23 native build and real-disc test pass: 42 galaxy records, 121 total
stars, 18 hidden stars and 7 Grand Stars. The existing
`smg-pc-game-data-real-or-absent-tests` build and run also pass, preserving the
selected-file session contract. This is storage validation, not gameplay or
persistent save-file validation.
