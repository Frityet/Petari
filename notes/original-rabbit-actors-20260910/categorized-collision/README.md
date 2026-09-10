# Original categorized line collision

This task restores `CollisionCategorizedKeeper::checkStrikeLine` from retail (`0x8017446C`, 700 bytes), then imports the complete native keeper source including the previously recovered `searchSameHostParts` and `getStrikeInfo`. Parent owns the MapUtil extraction, Binder inline, and global build; the depth agent owns CollisionParts line intersection and its return declaration.

The original method resolves the actual CollisionDirector, treats requested capacity zero as 32, clears its stored hit count, and builds a box enclosing the start/end points. Non-global zones must overlap both the box and the segment sphere test. It then traverses active parts in existing order, applies the actual optional parts filter, performs the same two broad-phase tests per part, and appends actual CollisionParts hits with the remaining capacity and triangle filter. Reaching capacity stores and returns the accumulated count immediately; exhaustion also stores and returns the count. It does not sort hits or choose a different collision category.

The existing native MathUtil declaration and original `checkHitSegmentSphere` provider cover the geometry dependency. CollisionParts must return a count: retail adds r3 into the keeper count. Its old reference `void` declaration is being corrected by the depth agent as part of the owned Parts recovery; native already declares `u32`.

`retail-byte-proof.json` verifies all 700 assembly bytes against actual main.dol, SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`. `checkStrikeLine-retail.s` preserves the bounded reference instructions.

## Frozen result

The first focused Wii compile/comparison passes: checkStrikeLine matches 100% (700 bytes); the previously recovered searchSameHostParts and getStrikeInfo remain 100% (120 and 16 bytes). `wii-proof.json` records the exact command and result. The full native TU also compiles successfully, recorded in `native-compile.json` and its log. No new tests, benchmarks, global builds, or commits were run.

The complete reference keeper source was copied into native, including both prior helper bodies and their actual includes. Both files have SHA256 `21b9a800ef44bc93e7a54fc4e46710b7fcab59fc37100ff7e7966168cbada5f5`; `source-manifest.json` records the paths and sizes. The only shared declaration prerequisite was the depth agent's reference CollisionParts return type correction, now saved. Parent owns linked runtime validation and activation with its rabbit/MapUtil/Binder cohort. A matching compile does not yet establish a playable chase.
