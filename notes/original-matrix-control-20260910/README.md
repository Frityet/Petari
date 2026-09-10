# Mario action-matrix startup hang

The demo's sampled startup stack stopped in `MarioActor::initActionMatrix` while `MatrixControl` repeatedly inspected the first row without advancing. Its constructor also passed an uninitialized row count to `HashSortTable` and omitted the original value-table allocation.

Recovered the constructor from the retail `Game/Player/MatrixControl.s` instructions under `notes/gateway-audit-20260907/restoration/retail/asm/`, following `decomp/AGENT_DECOMP_GUIDE.md`. Corrected the inverted found/missing condition in `getBitOrNone`. Audited the remainder of the file, including both `MatrixValueGetter` methods, against the same retail object.

## Behavior

- Inspect selector lists starting at index 1, retaining the original unsigned bound conversion. A selector with more than two choices switches the table to nibble values. This also preserves Mario's `-1` argument: the authored transformation selector table reaches a six-choice entry and stops the scan.
- Advance through map rows until the authored empty-string sentinel and use the resulting count to allocate and populate the hash table.
- For value tables, allocate eight bytes per row and unpack the packed 32-bit word from its high nibble to its low nibble. Bit tables keep their packed representation.
- Return `_1D` only when `getBitOrNone` cannot find the name; found names use their actual bit, including bits 0 and 31.
- Retain the existing `MatrixValueGetter` row scan, sorting after each insertion, optional output behavior, and retail destructors. Those already match retail exactly.

Reference source is in `decomp/src/Game/Player/MatrixControl.cpp` and was copied to `src/Game/Player/MatrixControl.cpp`. The only native difference is the six existing lookup output locals using `HashSortTable::Value`, preserving pointer-width writes on ARM64. No headers changed. `source-manifest.json` records exact hashes and verifies this correspondence.

## Verification and limits

- Fresh Wii compile: exit 0. Fresh native compile: exit 0.
- **Constructor: 100.0% retail match (376 bytes).** `getBitOrNone`: **86.77419% (124 bytes)**. Its remaining difference is branch-block layout: the compiler puts the missing-name return after the successful bit lookup; the conditions, bit operation, calls and returned values are equivalent. Natural explicit-result and opposite-condition formulations produce the same layout, so the final source keeps the direct guard.
- The other eight functions, including both `MatrixValueGetter` methods and both destructors, are **100.0%**. This is not a claim that the entire unit matches exactly. `retail-proof.json` lists every function separately.
- The isolated native probe builds and runs with AddressSanitizer and UndefinedBehaviorSanitizer: **3/3 groups pass**, covering multiple rows, all eight nibbles, the `-1` selector argument, bit-table allocation behavior, present/missing bit queries with both fallback values, scalar sorting/lookup, and optional/missing outputs. See `MatrixControlBehaviorProbe.cpp`, `probe-results.json`, and `probe-run.log`.
- The probe links the unchanged production `MatrixControl` and `HashSortTableCompat` implementations. Its explicit fixture-only `NameObj` methods omit scene registration, and its hash helper uses the production polynomial. It verifies the data algorithms and native widths, not scene lifetime or Mario movement.

The parent task owns rebuilding and relaunching the actual demo. Successful Mario startup, jumping, camera behavior, and scene teardown require that separate runtime evidence.
