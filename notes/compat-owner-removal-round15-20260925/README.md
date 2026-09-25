# Compat and scene removal, round 15

Removed ten compat files: collision-parts and HitInfo pairs, NAND provider/binding pair, the duplicate sound utility, original-joint-tree pair and alternate SceneObjHolder implementation. The alternate model renderer and its obsolete packet diagnostics are also deleted. src/compat now has 41 files.

The actual SceneObjHolder owns construction/rollback, its scene heap and child retirement. Its full donor switch keeps an explicit list of the 68 currently linked constructors for this batch. Actual LiveActor/CollisionParts own secondary parts and resources; complete donor HitInfo preserves original calculations with native lifetime checks. Aurora owns general NAND operations and descriptor lifetime, while SaveDataService retains GameData.bin/banner conversion. Complete current donor SoundUtil replaces its separately compiled copy, retaining eight required audio-disabled branches in the actual Game functions.

The user expanded scope during this batch: delete all src/scene too, and freely import the decomp source/header surface with marked stubs for unresolved gaps. Existing local scene-service deletions and dependent source/test migrations are therefore incorporated in this commit, rather than retaining obsolete HEAD-only APIs. Eleven src/scene files are removed from the committed baseline; 36 remain. Remaining ownership/catalog/collision removals are mapped in the next-scene notes. This is not a directory rename.

All current source/test changes are staged together through an isolated index so the committed code matches the build input. The six unrelated staged route notes, editor settings and historical note edits remain untouched. Individual lane snapshots still distinguish new edits from preexisting work incorporated under the expanded request.

Validation is intentionally limited to the integrated app build and fresh-save 120-frame Gateway opening run, with retries only for concrete integration failures. No standalone fixture suite or full Gateway/Rosalina route was run. Integration fixes and exact build/run results are recorded separately; no completion claim is made from a compile alone.

The final integrated build passed (build-retry4.log). The fresh-save Gateway opening completed 120 frames and exited 0 in 2.9 seconds. The binary SHA-256 stayed c821a9c18efe9c6132b7773e44d1d04b1702ee748ddf8605668f5d26a42e56b0 and no process remained. Four earlier compile failures were corrected at their actual declaration/layout/transaction boundaries; validation.json records each.
