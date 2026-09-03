# Original EffectSystem ownership and calculation — 2026-09-03

This checkpoint recovers the actual EffectSystem constructor, emitter creation, entry, AutoEffectGroupHolder and ParticleCalcExecutor. Root original-compiler and staged native compilation pass. No production activation, shared build, GPU work, or runtime ownership success is claimed yet.

## Root checkpoint: six files

- `src/Game/Effect/EffectSystem.cpp`: add constructor, createEmitter and entry; preserve all existing methods.
- `src/Game/Effect/ParticleCalcExecutor.cpp` and `include/Game/Effect/ParticleCalcExecutor.hpp`: complete seven-method class with actual host, four adaptor pointers and two flags.
- `src/Game/Effect/AutoEffectGroupHolder.cpp` and `include/Game/Effect/AutoEffectGroupHolder.hpp`: actual 256-entry fixed pointer vector, lookup, creation and three registration overloads.
- `include/Game/Effect/AutoEffectGroup.hpp`: actual group layout, a name and `MR::Vector<MR::AssignableArray<AutoEffectInfo*>>`, with declarations required by the holder. Its adjacent source method implementation is the next dependency closure; no substitute implementation is provided.

`root.patch` and exact `root/` source copies contain only these six files. Parent-owned ParticleEmitter/ParticleEmitterHolder and functional helper changes are only read while compiling, and do not belong to this patch.

All 9 EffectSystem methods and its vtable compare 100% against the retail object, including previously present createSingleEmitter. All seven AutoEffectGroupHolder methods and the exact original `std::find_if`/`MR::eq_ptr_case` instantiation compare 100%. ParticleCalcExecutor's constructor, five calculation/control methods, functor construction/call/clone and functor vtable compare 100%; `initMovementAdaptor` compares 99.88095% (string data placement). No layout blobs or typed dummy objects are used. Actual Wii sizes implied by those matching allocations are GroupHolder 0x404, Group 0x10, CalcExecutor 0x18, and DrawExecutor 0x24.

`verify-root.py` recompiles original root files using configured CodeWarrior flags. `verify-dol.py` independently confirms all 27 corresponding retail methods against local DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`, masking only explicit relocation fields. Full disassembly and `root-evidence.json`/`dol-evidence.json` are retained.

## Original ownership and scheduling

EffectSystem constructor initializes manager/draw/calc pointers, owns a new AutoEffectGroupHolder and creates actual draw/calc executors. The original constructor leaves the emitter-holder member to entry; callers must complete entry before emitter operations. This detail is preserved rather than adding a native bypass.

Entry creates `JPAEmitterManager(particleNum, emitterNum, MR::getCurrentHeap(), 9, 1)`, registers the supplied actual resource manager, constructs the actual ParticleEmitterHolder, and swaps its `IndDummy` texture using `MR::getScreenResTIMG()`.

The calculation executor's normal calculation updates the holder and invokes original JPA calculation separately for all nine groups, then consumes its update flag. Ignore-pause passes update the real holder and calculate groups 1 and 7. Its four NameObjAdaptors bind actual member functors to calc-animation types 19/20/20 and movement type 20. All original names and function-pointer targets were recovered from the DOL. The original `EffectSystem::init` is an actual four-byte return in retail; it is not a newly introduced empty API.

## Native staging and remaining providers

`build/original-effect-system-20260903/staged/` has literal root imports. `native-manifest.json` distinguishes six owned files from five dependency headers copied read-only for a consistent compile. `native-compiles.json` proves all three complete native TUs compile with native/Aurora header precedence. Do not apply read-only dependency headers blindly over parent integration.

The `*-undefined.txt` files are object-level dependency inventories, not a claim every item is missing from the complete application. The concrete adjacent missing implementations are:

- ParticleDrawExecutor's original constructor, draw methods and adaptor wiring; its current root source has only includes.
- Actual AutoEffectGroup constructor/add/registration helpers and AutoEffectInfo parsing methods; these are required by group registration, and currently only declarations exist.
- `MR::Effect::createParticleEmitter` and the screen texture service are reached by entry/createEmitter and must be backed by original resource/emitter and screen ownership.

These dependencies remain explicit. No fake emitter, null-resource shortcut, skipped texture swap, scheduler substitute, or Game API no-op is introduced. The parent owns integration with the original holder, resources and scene/screen services. Subsequent recovery proceeds separately so this source checkpoint can be reviewed atomically.
