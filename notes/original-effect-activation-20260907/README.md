# Original effect lifecycle and metadata restoration — 2026-09-07

Restored the complete original scene EffectSystem, ParticleEmitter, SingleEmitter,
MultiEmitter, EffectKeeper, calculation/draw executors, animation sync records,
auto-effect metadata groups, and utility translation units from the retained
preflatten source. Provenance and final original-compiler commands are recorded
in owner/ and utilities/. Code was restored here before mirroring into the port.

Retail comparison used the extracted RMGK01 DOL, SHA-1
25c5959534b3c21246c6c7e42021b916b41fb578, with Metrowerks GC 3.0a3 through
wibo/sjiswrap and original flags, exceptions off. Owner proof supplies226
functions (206 exact,224 at least90%). Utility proof supplies110 (97 exact,
109 at least90%), refreshed after the final shared template header repairs.

Two actual baseline omissions are repaired: child recursion in
MultiEmitter::createEmitterWithCallBack and forceDelete(EffectSystem*) now
matches100%. Ten newly reconstructed EffectSystemUtil methods restore original
lifecycle calls, floor-code attribute lookup, and allocation/initialization of
AutoEffectInfo. Original scalar/pointer field types are restored. HashSortTable
uses pointer-sized values on native platforms while retaining original Wii size.

Compilation repairs mirror the native legacy standard-library adapter: bind2nd
owns the converted second argument, const member-function adapters are provided,
and Array/Functor overloads avoid invalid eager instantiation. AutoEffectInfo's
original parser requires the standard strtoul declaration in cstdlib.

Low fuzzy scores were reviewed separately: the effectLight predicate differs
through tail-call optimization, a rotation constructor is inlined, and the
inherited addEffectHitNormal address scheduling differs. These reports establish
original code generation, not successful native rendering or complete gameplay.
