# RMGK02 decompilation and Mario demo readiness

Snapshot: `2026-08-09T03:24:14Z`

Repository commit: `28379387a966eeb7d910ce9378c63b17d0ee8e7d`

The canonical report was regenerated with:

```text
build/tools/objdiff-cli report generate -o build/RMGK02/report.json
build/tools/dtk shasum -c config/RMGK02/build.sha1
```

The rebuilt RMGK02 DOL passes the retail SHA-1 manifest. Its SHA-256 is:

```text
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf
```

## Aggregate progress

| Scope | Fuzzy code match | Strict matched code | Matched functions | Fully complete units |
|---|---:|---:|---:|---:|
| Whole executable | 79.00333% | 65.32589% (3,536,320 / 5,413,352 B) | 80.48340% (33,832 / 42,036) | 734 / 2,219 |
| Game | 75.78635% | 62.60914% (2,596,804 / 4,147,644 B) | 79.61366% (28,438 / 35,720) | 532 / 1,605 |
| Game/Player | 68.42615% | 31.61963% (148,912 / 470,948 B) | 59.24784% (961 / 1,622) | 10 / 107 |

`Fuzzy code match` measures instruction-level similarity and is the useful
functional-reconstruction indicator. `Strict matched code` credits code only
when it satisfies objdiff's exact matching boundary; a function can therefore
be behavior-complete and 99% fuzzy while contributing zero strict bytes.

## Walking-critical units

| Unit | Retail code | Fuzzy code match |
|---|---:|---:|
| GamePadUtil | 4,356 B | 98.69421% |
| MarioWait | 2,960 B | 98.63243% |
| MarioWalk | 8,860 B | 94.70700% |
| Mario2D | 1,008 B | 95.54365% |
| MarioMove25D | 1,976 B | 99.79757% |
| MarioSlope | 2,060 B | 99.63883% |
| MarioPress | 5,144 B | 99.90669% |
| MarioSideStep | 2,428 B | 99.96375% |
| MarioWall | 9,328 B | 93.43654% |
| MarioSpin | 1,412 B | 99.84986% |
| MarioDamageFreeze | 1,660 B | 99.97831% |

## Remaining source-absent Player units

The Player category has 107 configured translation units. Ninety-seven now
have source; these ten are still source-absent at this snapshot:

- `J3DModelX`
- `MarioDamage`
- `MarioBee`
- `MarioFoo`
- `MarioHang`
- `MarioRecovery`
- `MarioShadow`
- `MarioSkate`
- `MarioSpecial`
- `MarioTeresa`

Source presence does not imply runtime readiness. Important source-present but
incomplete units still include `Mario`, `MarioActor`, `MarioCollision`,
`MarioActorSensor`, `MarioActorShadow`, and the actor message/draw paths.

## Honest PC demo boundary

Already complete or available:

- exact StartInfo row ownership and retail Mario construction ordering;
- exact scene-owned `MarioHolder` prerequisite;
- exact stationed-player archive loading foundation;
- exact player-stick input surface;
- high-match wait, walk, wall, slope, press, side-step, and spin behavior;
- exact/no-fallback GX depth and resource-lifetime foundation.

Still required before advertising the `Mario` / `MarioActor` factory creators:

1. finish the constructor-eager Player state graph and critical actor methods;
2. bind the exact Mario model/J3D/animation path to the host renderer;
3. provide exact HitSensor/Binder/contact ownership backed by real stage KCL;
4. remove host-side double motion integration for exact actors;
5. mirror and link the Player closure into PC atomically, then register both
   retail creators;
6. use a debug-only placement preflight relaxation for a showcase build while
   keeping release strict-real-or-absent.

Audio is not on this critical path. A debug walking showcase is therefore not
one patch away, but it no longer requires finishing every Gateway placement
actor. It is principally one large Player-runtime closure plus one host
integration/acceptance wave.
