# Original SensorHitChecker integration — 2026-09-10

The complete original SensorHitChecker and HitSensor implementation now replace the scheduler's host all-pairs sensor evaluator. Category5 invokes the real registered checker through the ordinary original NameObj execution path. StopSceneStateControl can therefore stop and resume the actual owner, rather than dereferencing an absent SceneObj while native sensor work continues outside that owner.

The original checker retains its six authored group capacities and exact pair-pass ordering, same-host rejection, strict intersection condition, Eye one-way contacts and original validity/reclassification behavior. ActorRuntimeRegistry releases a keeper's sensors only after original invalidateBySystem removes their active group membership. Existing borrowed-contact removal on actor retirement remains unchanged. No broad actor, binder or collision-map changes were made.

The existing reference source needed two proven corrections: clear uses numeric0 for its u16 count (retail8016B6B0 stores halfword0), and the same-group inner loop begins at outer index i (retail8016BAC0 copies outer to inner), preventing duplicate pairs. checkAttack also now spells the exact retail arithmetic operand order. These changes were made in decomp before native copying.

One complete RMGK01 compilation succeeded. `wii-proof.json` records original object matching: group constructor/add/remove/clear, init, initGroup, movement, initHitSensorGroup and destructor **100%**; cross-group loop **98.333336%**, same-group loop **98.203125%**, checkAttack **99.62963%**, checker constructor **99.72973%**. No score tuning or additional test/benchmark suite was run.

## Native lifetime / parent integration

`SensorHitChecker` already declares a real virtual destructor. The imported native Game TU excludes only its original empty destructor under TARGET_PC; `compat/SensorHitCheckerOwnership.cpp` defines that destructor to free the six original group arrays and objects. This replaces the Wii whole-scene heap reclamation without introducing a replacement Game class or process registry.

Parent adds the real factory case and creates the checker after SceneExecutionBinding has published its actual executor, before any HitSensor constructor. It must outlive scene actors and their sensor keepers; reverse creation-order retirement provides that relationship. No additional lifetime API is necessary. The native CPPs are discovered by existing source globs. No Xmake was run by this agent; the combined build and short smoke remain parent-owned and unclaimed here.

`closure.md` preserves the initial read-only audit; `source-manifest.json` captures this agent's exact frozen cohort. Parent-owned factory/prerequisite changes are separate.
