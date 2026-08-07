# Real MarioActor closure handoff

Audit time: 2026-08-07T04:35:54Z

## Outcome

`compat/StagePlayerRuntime` cannot be replaced by copying only `MarioActor.cpp`, and it must not be replaced by another synthetic player. The constructor-seeded retail link graph requires 96 Player translation units:

```text
67  have current decompiled source
29  exist only as extracted original PPC objects
96  total real-player closure
```

Those 96 objects then require 830 symbols outside Player:

```text
665  Game
 53  JSystem
 82  RVL SDK
 30  compiler/MSL runtime
```

The current correct interim behavior is therefore: the host may load stage metadata, but the player is absent. It must not create a movement proxy, force title rush, synthesize a model, or treat a start position as a real `MarioActor`.

## What the RMGK02 build proves

`build/RMGK02/main.dol` passes the configured SHA-1 check exactly. All 78 currently present decompiled Player `.cpp` targets are current under the RMGK02 Metrowerks build.

The final DOL is hybrid. Only 10 of 107 Player objects are linked from decompiled source, and only eight of the 96 objects in this Mario closure are source-linked; the other 88 closure objects come from extracted retail PPC objects. RMGK02 is a valid symbol/behavior oracle, not proof that the full real player exists as portable source. See `build-proof.md`.

## No-stand-in integration path

1. Decompile all 29 missing Player files listed in `source-closure.md`. Compile each under RMGK02 and extend the source/header dependency scan. Until then, keep the PC player creator disabled.
2. Restore the exact shared Game foundations that the player consumes: `NameObj`, `Nerve`/`Spine`, `LiveActor`, binder/collision/sensors, model manager, animation, effect keeper, parts model, and their utilities. Host-side storage and platform behavior belong behind generalized compatibility services.
3. Expand Aurora and the PC JSystem substrate for the real retail APIs and semantics: GX/GD paths and state, matrices, JGeometry/JMath, J3D model/animation, heap/aligned allocation, OS, pad/swing input, and a safe 64-bit address model. Do not put these accommodations in Player.
4. Add the complete 96 Player sources and 63 known Player headers without gameplay changes. Compile and link them while the placement creator remains disabled. The known minimum transitive Game-header surface is 197 headers and will grow when the missing source is recovered.
5. Resolve the 830 external symbols by their real subsystem, following `unresolved-symbols.md`. A symbol that only links to a no-op, guessed return, dummy transform, fake resource, or player-specific shim is still unresolved under this rule.
6. Enable the retail lifecycle through the generalized placement/NameObj factory path. Create `SceneObj_MarioHolder`, construct `MarioActor` for a real `Mario`/`MarioActor` placement, call exact `init(JMapInfoIter const&)`, and let exact `init2` register the actor, sensors, binder, scheduler categories, rendering, and camera target. Do not construct it directly from `StageStartInfo`.
7. Validate the complete title -> file select -> picturebook -> gateway sequence. Title activation must be caused by the real sensor/SpinDriver/MarioActor rush messages, never a timer or forced route transition.
8. Compare native behavior to RMGK02 in the cloned Dolphin build: placement transform, base/joint matrices, binder radius/offset and triangle contacts, sensor types/messages, Nerve transitions/steps, animation names/frames, spin/rush input timing, camera target, effects, and scene transition events.

Any intermediate build that cannot satisfy a step should leave the player or feature absent and report that absence. It should not invent fallback behavior.

## Evidence map

```text
build-proof.md                         verified RMGK02 DOL and hybrid-link caveat
closure-method.md                      reproducible object-graph method and totals
source-closure.md                      exact 67 present + 29 absent source inventory
known-minimum-game-header-closure.txt  197-header minimum Game dependency list
unresolved-symbols.md                  830-symbol external subsystem inventory
host-probe.md                          native boundary/compiler blockers
probe/                                 audit-only retail-include forwarding evidence
```

No production source was edited by this audit.
