# Original stage effects, 2026-09-13

Restored the complete StageEffectDataTable implementation in decomp first, then copied the same source/header to the native Game module. This is the shared dependency used by original MapObjActor movement, not a Gateway substitute.

Recovered 22 retail functions: three table lookups, camera strength routing and player-distance gating, sound accessors, data/type queries, start/moving/stop camera and rumble requests, and the original particle/sound event dispatch. Preserved all 155 existing sound rows and recovered all 32 camera and 29 pad rows from the retail relocations and data words. Camera ranges retain the original -1 standing-on-player meaning. Rumble strings are decoded from retail Shift-JIS. No authored row was invented or selected specifically for the demo.

The original compiler succeeds. First comparison gives 16 functions at 100%, three lookup functions at 92.32%, and the three top-level dispatch wrappers at 84.75%, 63.84%, 84.75%. Those wrappers call the identical recovered isExistStageEffectData function where retail inlined it; the guards, parameter values, sound/effect requests and request order are unchanged. No matching-polish campaign was performed. Reference summary and exact compile commands are retained here. Native compilation succeeds; sources/header are identical between the two trees. An explicit switch default simply returns for the original unsupported/zero strength case.

This supplies real shared source for the remaining map-object dependency graph. It does not activate a placeholder factory and does not establish Gateway placement, wakeup, bunny progression or Rosalina runtime success.
