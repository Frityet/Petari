# SMGCommunity integration — 2026-09-10

Merge SMGCommunity/Petari 16906807c69697fdea5d7efbb4642313c1b14204 into pcp-decomp, preserving complete locally recovered original functions. There are 104 incoming commits since common base 151d5a53a. Safety ref codex/pre-cleanup-upstream-20260910 retains db38bcbac05d051fa9b319306a7ba43cb84f37ae.

Mario.cpp is resolved per function: preserve verified input thresholds, grounding result, gravity-orientation predicates and pose logic; adopt the missing original physical writeback bit13 branch (97.61364 to 98.42975 percent Wii match). Retain complete local MarioMove, sensor, Special, particle and water implementations where incoming code remains partial. Adopt disjoint area helpers and original field names. No duplicated Mario definitions remain.

Correct SwitchWatcherHolder::movement to iterate the populated range captured before callbacks, as retail 8019F58C–8019F5C0 requires (100 percent match). Remove 13 redundant dereferences of getAirGravityVec(), which already returns a vector reference, after incoming pointer conversions introduce compiler ambiguity. All four affected function scores improve and MarioCollision::checkGround remains byte-identical.

Recover LightAreaHolder::sort against retail 80023670 (160 bytes). Full Wii object compiles; function match 96.375 percent. This recovers the implementation previously present only in the port and preserves original selection ordering.

Validation: 276 impacted incoming/local Game translation units compile with the original RMGK01 compiler; LightAreaHolder compiles separately after recovery. This is object-level validation, not a full original executable link. The accompanying native integration passes its 960-tick real-disc movement/camera replay (all 13 checks, approximately 60 FPS). Full Gateway bunny chase and Rosalina sequence remain outside this checkpoint.
