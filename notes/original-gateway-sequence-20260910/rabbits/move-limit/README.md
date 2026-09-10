# Original movement-limit collision prerequisite

Recovered reference sources are frozen. No native source, root build configuration, index, or commits were changed. The 22-file rabbit import remains held while the parent finishes the demo/talk checkpoint.

`MR::trySetMoveLimitCollision` now follows retail at `0x803E2AD0`: start at actor position minus 150 times gravity, cast 1000 times gravity into category 3, and bind the first hit's collision parts. If that misses, cast into category 0 and find the first category-3 part with the same HitSensor host. The latter branch sets the resulting pointer even when the search fails, and returns true because the map cast hit. Only two failed casts return false. The original code assumes an initialized Binder and collision owner.

The required `Binder::setExCollisionParts` inline was missing its flag writes. It now assigns the pointer and clears/sets `_1EC._2` for null/non-null respectively, matching retail. The small `CollisionCategorizedKeeper::searchSameHostParts` and `getStrikeInfo` prerequisites were also recovered: the search traverses the existing zone/parts order, compares actual sensor hosts, writes the first match, and leaves the output untouched on failure. No synthetic collision owner or missing-hit fallback was added.

## Compact proof

Both complete changed reference translation units compile successfully. `final-proof.json` records the commands and object comparisons:

| Function | Retail bytes | Match |
| --- | ---: | ---: |
| trySetMoveLimitCollision | 332 | 99.87952% |
| Binder::setExCollisionParts | 44 | 100% |
| searchSameHostParts | 120 | 100% |
| getStrikeInfo | 16 | 100% |

The only remaining MapUtil differences are names/placement of the two float constant-pool symbols. `compiled-constant-proof.json` confirms the compiled operands are exactly 150 (`43160000`) and 1000 (`447a0000`). `retail-byte-proof.json` verifies all 512 assembly bytes and those two constants against the actual main.dol, SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`.

`source-manifest.json` and `reference.patch` identify the three changed reference files. The first compile of the MapUtil candidate preceded the newly identified Binder correction; the final proof supersedes that intermediate object. Existing rabbit proofs were reused without repeating Wii builds or gameplay tests.

## Native integration frontier

Native Binder remains unchanged as requested, so its inline correction must accompany the recovered caller's later integration. Native MapUtil is currently excluded and its MR queries are supplied by generalized collision compatibility; importing only this recovered body outside Game would avoid reintroducing duplicate providers, or the parent can activate a coherent larger original source cohort.

The next concrete original dependency is `CollisionCategorizedKeeper::checkStrikeLine` (`0x8017446C`, 700 bytes), currently absent in both source trees. It in turn calls the missing `CollisionParts::checkStrikeLine` (`0x801771A4`, 492 bytes). Existing host line queries do not implement these class methods. This is a shared original collision path, separate from simply importing the rabbit actors, and has not been replaced by a Gateway-specific query.
