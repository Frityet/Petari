# Original effect owner activation — 2026-09-07

The upstream compilation audit exposed missing original effect APIs, but the
existing host implementation supplied only event metadata and its own particle
simulation. `LiveActor::initEffectKeeper` left `mEffectKeeper` null. Returning a
fake `MultiEmitter` from that service would not provide the original callback,
emitter-link, or lifetime behavior.

The implementation direction is to restore the previously recovered original
EffectSystem / ParticleEmitter / SingleEmitter / MultiEmitter / EffectKeeper
code, retain actual JPA resources and scene storage, and retire the former
actor-facing effect simulation when the original owner is ready. Game source
remains literal except required architecture or compile changes. Process
particle resources and screen capture textures must outlive scene emitters;
emitters must retire before their callbacks and actors.

`restored-source-manifest.json` records exact original source provenance from
the preflatten Git tree, the old decomp/native hashes, and restored hashes.
Restoration writes `decomp/src` / `decomp/include` first, then mirrors those files
into native `src`. The actual guide read is `decomp/AGENT_DECOMP_GUIDE.md`.

Historical packages inspected for evidence:

- `notes/original-effect-system-native-20260903`: complete native manager graph,
  actual-disc calculation cycles, actual renderer-backed entry/screen texture
  proof, explicit owner activation requirements.
- `notes/original-multi-emitter-callback-native-20260903`: original callbacks on
  actual JPA emitters, verified transform/color behavior and lifetime ordering.
- `notes/original-effect-keeper-completion-20260903`: original keeper methods and
  pointer/value-safe legacy binder implementation.

These historical tests establish provenance and an implementation route. Fresh
production activation and runtime results will be recorded here as they run.

## Active native owner checkpoint

`SceneObjHolderBinding::initialize_effect_system` constructs the original scene
system with the original default budgets (3072 particles, 256 emitters) in a
retained 8 MiB JKR arena. The process particle catalog is retained unchanged.
The scene owns its scheduler registration suffix, including predraw callback
history, and removes it before emitter callbacks, actors, adaptors or the arena
can be reclaimed. The parent wired Title, Stage and Gateway hosts.

`EffectSystemOwnership` captures the original raw owner graph for native
retirement. Actual EffectKeeper/MultiEmitter/SingleEmitter objects now populate
LiveActor::mEffectKeeper. Native actor teardown first retires every associated
real JPA emitter, then releases callbacks, animation sync records, emitter
arrays and keeper lookup tables while model/resource pointers remain valid.
The original LiveActor update, clipping, death and binder effect calls are
restored. The prior actor metadata-only EffectService registration was removed;
layout effect adaptation remains a separate existing surface.

Original repeated one-shot emission clears its previous SingleEmitter link
while the previous particle emitter still borrows its callback. Native
JPABaseEmitter preserves the latest nonzero opaque user-work token until slot
reinitialization; its public user-work field preserves the original semantics.
Owner retirement uses those exact SingleEmitter addresses to delete orphaned
instances without deleting unrelated actors' effects. This metadata is confined
to the native SDK and is reset on every real emitter initialization.

The current ownership regression passed two complete real-disc scenes and two
RuntimeContexts, with exact host-heap free bytes and NameObj counts restored.
It checks callback initialization/execution, orphaned one-shot retirement,
unrelated owner preservation, slot reuse, double-calculation suppression,
child callback propagation/deletion, surviving actor pointer clearing, and
automatic scheduler/predraw/domain retirement. Logs are ownership-build.log and
ownership-run.log. The fixture explicitly selects an immediate-start one-shot
with a lifetime exceeding its fixed-frame assertions; initial catalog entries
can be delayed or shorter-lived by authoring, so arbitrary first-row selection
was invalid for those assertions.

## Fresh original-source proof

`verify-wii.py` compiles all 14 original owner translation units with current
Metrowerks GC 3.0a3 through wibo/sjiswrap, then compares original retail objects.
`wii-compile-results.json` saves exact commands, hashes and full per-symbol
results. `wii-proof.md` and `wii-proof-summary.json` summarize 226 supplied
function symbols: 206 score 100%, 224 score at least 90%. Direct instruction
audits explain the two low scores (short-vector constructor inlining and the
retail near-zero-only effectLight predicate's tail-call optimization).

This audit found and fixed two real omissions in the recovered baseline:
MultiEmitter::createEmitterWithCallBack and forceDelete(EffectSystem*) both
omitted their original child-emitter traversal. They now each match retail
100%, and the native child regression verifies their actual behavior.
The const-element SRT matrix signatures now match retail. Decomp-only template
compile repairs mirror existing native behavior: deferred FixedArray member
function typing, const-receiver Functor overload, const mem_fun_ref, and a
bind2nd factory that retains the converted second argument by value.

## Native lifetime and renderer validation

The extended real-disc test now builds and passes two complete RuntimeContext
and scene cycles on native arm64 LLVM23. Each cycle restores the host root heap
free size, NameObj registry count, and scheduler registration baseline exactly;
both scene arena weak references expire. Failure cases validate a rejected
32-byte budget, 512-byte constructor exhaustion, and 4096-byte entry exhaustion.
Actor destruction removes callback-bearing orphan emitters without removing an
unrelated actor's emitters. Pool reuse clears the retained token. Original child
callback creation and force deletion are exercised recursively.

A separate real particle tranche submits twelve renderer frames using actual
Effect.arc resource 2PGlowActiveLoop00, the original EffectSystem scheduler,
ParticleDrawExecutor draw category71 and JPA draw path. In each cycle it observes
7 live particles, 9 GPU draw calls, and 2,391,153 changed RGBA bytes versus the
cleared baseline. The display-copy screenshot is
../original-effect-ownership-20260907/render.png and visibly contains textured
green particles. This is a controlled renderer fixture, not a full-game demo or
Wii pixel-parity claim. Commands and results are in ownership-build.log and
ownership-run.log. The expected allocation-failure diagnostic lines are part of
the negative cases, not a failed final result.

SceneObjHolderBinding creates a separate 8 MiB Game arena for actual runtime
scenes and holds SceneSchedulerAllocationBinding until typed scene teardown
finishes. EffectSystem's pool arena remains separate. Original scheduler
callbacks enter the Game arena while native bookkeeping escapes heap routing.
The regression invokes original MR::addEffect twice inside an ordinary NameObj
movement callback and verifies exact JKR heap provenance for both MultiEmitters
and both successive AutoEffectInfo allocations, including the replaced record.
Dynamic EffectKeeper::addEffect can orphan prior records on duplicate names;
those allocations now belong to the scene heap, without guessing ownership of
borrowed MultiEmitter::_28 fields or replacing the original method.

A separate lane is extending the host allocation escape into Aurora native
renderer caches/FIFO paths. The bounded two-cycle test passes, but should not be
interpreted as proof that every possible backend capacity-growth path already
escapes original Game heap routing. Parent coordinates validation after that
change.

Explicit coordinated commit lists are decomp-paths.txt and native-paths.txt;
the parent owns shared Git indexes, build scheduling, and commits.

## Extended backend checkpoint

The subsequent64-depth-snapshot extension passes all assertions: no Game heap
space is consumed by persistent native snapshot/FIFO allocations, caller routing
is restored, and backend records remain queryable after both scene arenas expire.
However the first process then aborted during Dawn asynchronous map completion
at final shutdown. Five subsequent executions exited0. This intermittent backend
shutdown issue is unresolved and the extended executable is not claimed reliably
passing yet. See ../compat-final-validation-20260907/README.md and the sanitized
native crash summary; the backend lane owns the fix.
