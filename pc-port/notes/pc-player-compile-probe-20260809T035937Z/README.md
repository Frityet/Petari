# Exact Player PC compile/provider frontier

Audit time: 2026-08-09T03:59:37Z through 2026-08-09T04:10Z.

This is a diagnostic-only audit. It does not copy Player into `pc-port`, enable
the Mario/MarioActor creators, change `Game` behavior, or change xmake.

## Outcome

The constructor-seeded retail object closure remains exactly 96 Player TUs.
The current root source frontier is:

| Gate | TUs | Percent of closure |
|---|---:|---:|
| Native-parseable against current PC-first headers/Aurora | 52 | 54.17% |
| Source present, native syntax blocked | 41 | 42.71% |
| Source absent | 3 | 3.13% |
| Total | 96 | 100% |

The three source-absent units at the frozen snapshot are `MarioHang`,
`MarioShadow`, and `MarioTeresa`. `MarioFoo` appeared during the audit and is
included in the 93 attempted source TUs. See `frontier-matrix.tsv` for all 96
rows and `first-errors.tsv` for the deterministic first error of each blocked
TU.

The 52 passing TUs also compile to isolated native objects. Together they
define 696 symbols and request 413 external native symbols. Only 119 of those
413 names are found in the current PC debug archives; 294 are not (14 of the
294 are ordinary native compiler/system runtime names). This is deliberately a
partial-subset link observation, not a claim about the uncompiled 44 TUs.

## Syntax frontier

First-error attribution across the 41 blocked source TUs:

| First blocker | TUs |
|---|---:|
| J3D/model/animation | 13 |
| RVL/GX/GD platform surface | 10 |
| Other exact Game declaration/include surface | 9 |
| Binder/KCL | 4 |
| Modern C++/64-bit compiler compatibility | 3 |
| Effects/audio | 2 |

All-error TU incidence (a TU can occur in more than one row) is 20
J3D/model/animation, 19 other Game/API, 11 RVL/platform, 8 modern C++/64-bit,
6 Binder/KCL, 3 effects/audio, 2 resources, and 1 HitSensor.

The shared deterministic blockers are:

- Aurora exposes only its sized PC `GXSetArray` form while exact J3D calls the
  retail three-argument form; its anonymous GX enums also omit the retail
  `_GXAttr` and `_GXTlutSize` tags.
- the PC J3D/JGeometry surface lacks complete `J3DModelData`/animation types
  and exact operations such as `TUtil::PI`, `JMath`, `TVec::zero`, `set2`, and
  `mult`, `TRotation::getEuler`, and matrix assignment from `MtxPtr`;
- PC `LiveActor` omits exact members including `mBinder`, `mCollisionParts`,
  `mEffectKeeper`, and `mModelManager`;
- exact effect/resource declarations are not present for `MarioEffect` and
  `MarioSound` (pure audio can remain deferred, but EffectKeeper is not an
  audio-only concern);
- host compilation still exposes aligned `new (0x20)`, 32-bit pointer storage,
  case-sensitive `KariKariDirector.hpp`/`KarikariDirector.hpp`, and control-flow
  crossings accepted by Metrowerks.

`Nerve.hpp` and `Spine.hpp` are already byte-identical between root and PC.
The actor ABI gap is specifically the divergent `NameObj` and `LiveActor`
layout/surface, not a current Nerve/Spine header gap.

## Full retail provider ledger

The complete 96-object retail closure still defines 3,175 symbols and leaves
exactly 830 symbols outside Player, supplied by exactly 146 selected retail
provider objects. `provider-ledger.tsv` maps every symbol to its provider,
category, and current PC source status.

| Provider category | Symbols |
|---|---:|
| J3D/model/animation | 245 |
| Other Game/runtime | 195 |
| LiveActor/NameObj/Nerve ABI | 91 |
| RVL/platform | 82 |
| Effects/audio | 74 |
| Binder/KCL | 60 |
| HitSensor | 39 |
| Compiler/MSL runtime | 30 |
| Resources | 14 |
| Total | 830 |

For the 665 Game-provider symbols specifically, the current PC tree has:

| PC Game provider state | Symbols | Provider objects |
|---|---:|---:|
| Source absent | 289 | 60 |
| Root byte-identical source present but excluded by Game xmake | 257 | 20 |
| Divergent PC source included | 83 | 12 |
| Root byte-identical source included | 36 | 6 |

The remaining providers are 53 JSystem symbols/19 objects requiring a PC API
audit, 82 RVL symbols/23 objects whose Aurora subsystems exist but whose retail
surface is incomplete, and 30 compiler/MSL symbols/6 objects mapped by the
native toolchain. Presence of a divergent or excluded source is not counted as
semantic resolution.

## Ranked smallest generalized tranches

1. **Restore retail GX declarations in Aurora.** Preserve the `_GXAttr` and
   `_GXTlutSize` enum tags and support retail `GXSetArray(GXAttr, const void*,
   u8)` while retaining the sized native upload implementation. The
   diagnostic overlay unlocks 5 TUs with no regressions, moving the frontier
   from 52 to 57.
2. **Restore exact `NameObj`/`LiveActor` ABI and move host storage behind a
   generalized registry.** The exact-header overlay alone unlocks 4 TUs
   (`MarioCollision`, `MarioEnforce`, `MarioSpecial`, `MarioWalk`) with no
   regressions. Combined with the GX tranche it unlocks 9 and moves the
   frontier to 61. This is the mandatory semantic spine before any creator can
   be enabled even though J3D/GX is the larger raw syntax category.
3. **Complete the generalized PC J3D/JGeometry/animation surface.** It owns 13
   first blockers and 245 of the 830 retail external symbols, making it the
   largest next compile/link tranche. Start with complete `J3DModelData` and
   the exact vector/matrix/JMath operations used by Player, then bind those APIs
   to the existing renderer rather than adding Player-local substitutes.
4. **Bring Binder/KCL and HitSensor through the exact actor surface.** They
   account for 99 retail external symbols. Exact `LiveActor` makes the public
   ownership slots available; the generalized keepers/binder/collision
   providers must follow before movement can be meaningful.
5. **Resolve effects/resources, then modern-compiler/address-model issues.**
   Pure audio can stay absent. Effect/model callbacks, aligned allocation, and
   pointer-width correctness cannot.

The smallest useful implementation tranche is therefore the exact actor ABI
plus the two Aurora GX source-compatibility contracts. It is still a
compile-only tranche: keep Mario/MarioActor placement creators disabled.

## Reproduction and evidence

From the repository root:

```text
python3 pc-port/notes/pc-player-compile-probe-20260809T035937Z/closure_ledger.py
python3 pc-port/notes/pc-player-compile-probe-20260809T035937Z/compile_probe.py
python3 pc-port/notes/pc-player-compile-probe-20260809T035937Z/provider_ledger.py
python3 pc-port/notes/pc-player-compile-probe-20260809T035937Z/native_subset_link.py
python3 pc-port/notes/pc-player-compile-probe-20260809T035937Z/measure_rvl_tranche.py
python3 pc-port/notes/pc-player-compile-probe-20260809T035937Z/measure_actor_abi_tranche.py
```

The scripts use isolated `g++ -fsyntax-only`/`-c` commands and the existing
debug archives; they do not reconfigure or build the production target. The
`revolution-overlay`, `rvl_compile_overlay.hpp`, and `exact-actor-overlay`
directories are diagnostic-only declaration selectors. Raw per-TU logs are in
`compile-logs/`; summary JSON and TSV ledgers are alongside this README.
