# Original emitter linkage

The four recovered MR::Effect functions create the actual JPA emitter, retain
its actual wrapper identity, store/retrieve the SingleEmitter pointer in JPA
user work, and request normal deferred deletion while resuming calculation.
Failed creation preserves the wrapper. Native pointer storage uses uintptr_t;
the Wii code remains exactly unchanged in size and instructions.

All four methods compile at 100% with GC3.0a3. `verify.py` verifies every
relocated instruction against the known RMGK01 DOL: 180 bytes total. No
animation, rate, actor, stage or resource override is introduced. Native Game
effect ownership and API activation remain a separate integration step.
