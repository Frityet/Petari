# Original embedded StoryEventBCSV restoration

Restored the exact original **480-byte StoryEventBCSV** at retail address **8053DC20** in `decomp/src/Game/System/GameDataHolder.cpp`, following `decomp/AGENT_DECOMP_GUIDE.md` and the native GalaxyIDBCSV data/registration precedent. The empty `const static JMapInfo` placeholder is replaced with an aligned, externally visible constant byte array. No holder gameplay statement was changed in this task.

The bytes come from the retained retail object/assembly, not reconstructed rows. Table SHA-256 is `8f508f1cafc925ff60fb774e043a6e89cb4ba4cc136e591dde29399efad404ea`. All 14 CP932 name/progress rows exactly match the previously verified native registry; `decoded-retail-table.json` preserves their provenance. `proof-summary.json` verifies that the new source initializer equals the extracted retail bytes.

The corrected whole GameDataHolder.cpp was copied to `src/Game/System/GameDataHolder.cpp`. Before copying, its native differences were inspected: an equivalent spelling of the corrected story comparison, and the older incorrect flag-bit updater. There were no native ABI or compile-only modifications to preserve. The native and decomp files are now byte-identical, including both prior reference recoveries.

`EmbeddedGameTables` now owns a JMapResource and original-address source registration for the complete StoryEventBCSV extent, using the same mechanism as GalaxyIDBCSV. This keeps bounds, decoded metadata, CP932 conversion and attached-reader lifetimes in the existing general resource layer. Aliases retire before their corresponding JMapResource member. There is no special-case handling of the old empty object's address or invented story row.

## Verification

- Fresh **whole-TU Wii compile exits 0**; exact command and log in `wii-compile.json`/`.log`.
- Fresh retail objdiff of **StoryEventBCSV: 100%, 480 bytes**; data comparison command in `data-objdiff-command.json`.
- Original **GameDataHolder constructor: 100%, 440 bytes**. Story predicate remains 100%; flag-bit updater remains 92.06896%, with the previously documented register/scheduling differences. The unchanged `getPictureBookChapterCanRead` body compares at 99.42857% instead of 100% after the additional rodata changes relocation matching for its existing chapter-suffix reference; the helper was not edited. The whole rodata section is deliberately different because it now contains the restored 480-byte object, which is compared independently at 100%.
- Native **EmbeddedGameTables syntax check exits 0**, using current production headers and options; command/hashes in `registration-native-syntax.json`.
- Native/reference whole-file equality and both repository `git diff --check` checks pass.
- `source-manifest.json` records exact source, header, candidate-object and retail-object hashes.

**No native GameDataHolder activation, native link or runtime test is claimed.** GameDataHolder.cpp remains excluded by the existing build configuration. The new resource registration references the exact table defined in that TU, so the parent integration must enable its actual whole-owner dependency cohort before a final executable can link. This task did not add another table provider, enable Xmake, edit the host facade, or create a partially initialized owner.

Only the decomp/native GameDataHolder.cpp pair and `src/resource/EmbeddedGameTables.cpp/.hpp` were edited. No headers in Game, build files, commits or index state were changed by this task. Large objects and full objdiff output are local evidence; the README, compact JSON proofs and manifest are the intended review notes.
