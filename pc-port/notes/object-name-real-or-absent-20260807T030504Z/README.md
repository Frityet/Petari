# Object-name real-or-absent cleanup

## Outcome

Placement construction now keeps the two retail inputs separate:

- `placement.object_name` remains the English key passed to `NameObjFactory::getCreator`.
- the actor/display name is the nullable `jp_name` result from `ObjNameTable.tbl`.

When `ObjNameTable.tbl` has no row for a supported placement object, the creator is still called with the English object key and a null display name. The English key is never substituted as the display name.

The obsolete `ObjectNameTable::lookup_or_self` API and the `object_name_table_fallback` trace state were removed. Missing rows now produce the debug-only `object_name_table_absent` trace.

No files under `pc-port/src/Game` were changed for this cleanup.

## Retail evidence

`src/Game/Scene/StageDataHolder.cpp` implements `StageDataHolder::getJapaneseObjectName` by returning `nullptr` when the `en_name` lookup reaches the end of the table. It does not return the English input.

The RMGK02 assembly in `build/RMGK02/asm/Game/Scene/PlacementInfoOrdered.s` confirms the caller order:

1. `803429D4`: resolve the creator from `PlacementInfoOrdered::Identifier::mName` (the placement object name).
2. `803429E4`–`803429E8`: call `MR::getJapaneseObjectName` with that name.
3. `80342A0C`–`80342A18`: invoke the already-resolved creator with the returned pointer, including null.

This is why absence of a localized label must not change factory support.

## Regression coverage

`ObjectNameTableTests.cpp` now proves all of the following:

- known rows return their decoded table-owned display name;
- an unknown row returns null, with no lookup-or-self API available;
- an explicitly empty `jp_name` remains a present empty mapping;
- a real `Coin` factory remains constructible when the synthetic table has no `Coin` display-name row;
- the resulting actor name is null/empty rather than the English `"Coin"` identifier;
- the extracted RMGK01 and RMGK02 tables still contain and decode all 1,691 mappings;
- known absent retail rows remain absent.

See `verification.log` for commands and results.
