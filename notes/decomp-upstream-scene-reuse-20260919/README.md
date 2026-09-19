# Upstream Petari merge and native reuse

Merged 274 upstream commits through `7663bd3be278c7dfaaccd9df729b3b9cf61acf27` into the cloned decomp's `pcp-decomp` branch. The merge is `ce9b333a05d42e4e2ecf2ccb050ce21ea4d04e3b`, pushed to Frityet/Petari and verified against the remote branch SHA. It preserves local recovered bodies wherever upstream remains incomplete; 159 conflicts were reviewed across the actor/collision, player/system, scene/demo and utility/library families.

## What was used in the PC port

Applied three upstream DemoRabbit corrections, each verified against the retail DOL: collision rebound coefficients `(0, 0, 1)`, wall-jump hit-power comparison `>= 0`, and goal animation `Wait`. These are corrections to original recovered code, not compatibility special cases. The patch and byte/address evidence are in this folder.

Removed 2,933 lines (28.6%) from `src/scene`: duplicate placement orchestration, static collision BVH/query algorithms and the custom scene executor. Original StageDataHolder/PlacementInfoOrdered, CollisionDirector/CollisionParts/KCollisionServer and SceneFunction own those behaviors. Details and fixture migrations are in `../scene-original-owner-reduction-20260919/`.

## What upstream makes available next

The merged typed StageDataHolder/PlacementInfoOrdered family is useful for replacing remaining placement inspection adapters, but native already has the recovered production traversal. The complete FurCtrl/FurMulti/J3DVtxShader family, new collision/material helpers and additional map utilities are available for focused native imports. LayoutManager and effect changes require checking host resource ownership, pointer lifetimes and endian handling before importing; neither all incoming Game files nor all newly decompiled actors are enabled on PC by this merge.

Local ParticleDrawExecutor, original collision/recovery routines, DemoSound leading-byte behavior, five ObjUtil functions and other recovered bodies were preserved. Incoming aggregate matching percentages were not accepted as proof of individual branches; the DemoRabbit and AutoEffectInfo decisions include direct retail evidence.

## Validation

All 1,603 configured Game C++ translation units and all 35 changed non-Game C/C++ library units compile using the configured Wii MWCC commands. The final receipt verifies the source hashes of all 1,638 successful units. One unconfigured Color.cpp contains only an include and commented-out function. This is compilation evidence, not a retail link or exact matching claim. Initial failures and subsequent repairs are retained as separate batches.

The native app completed a fresh 600-frame Gateway startup with these source changes, exit 0, unchanged binary hash and a reaped process. Original player/collision-area/sphere-query/rotating-map-object probes pass. Startup screenshot and machine-readable receipts are in `../original-map-object-closure-20260919/` with prefix `upstream-scene-smoke`. This run does not prove the complete chase or visual parity through Rosalina.

Upstream's existing whitespace defects were retained without broad formatting churn; `staged-whitespace-provenance.json` identifies every reported line. The preexisting untracked NPCUtil.d and unrelated parent work were preserved.
