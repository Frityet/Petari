# Root Mario update restoration — 2026-09-03

Restored `Mario::update`, `Mario::inputStick`, and
`Mario::checkForceGrounding` in **root** `src/Game/Player/Mario.cpp`.
The six-method group now compiles using the configured original compiler and
compares against the current verified Korean retail executable. No PC Game
mirror, native movement replacement, tests, or build configuration was changed
in this tranche. This is source recovery needed before replacing the existing
grounded-only PC update path; it does not establish playable jumping yet.

## Provenance and verification

Historical starting source is commit
`96e5ef0decce22e5bfd7d0ee876fb15ac80a725b`,
`src/Game/Player/Mario.cpp`, SHA-256
`bd3fd8040019266aeeb31020b765c3127c290e9c72610efb6f7c8081110e66a2`.
The baseline for this edit is commit
`5942655dbea3909861069bd4214203f44dd5eda9`. The historical recovery note was
`notes/mario-walking-core-rmgk02-20260809T043114Z/README.md` and described
**RMGK02**, so its earlier percentages were not used as current verification.

The live target here is `build/compat-math-oracle/main.dol`, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`, matching
`config/RMGK01/config.yml`. `dtk v1.8.3 dol split --no-update` uses the actual
`config/RMGK01/symbols.txt` and `splits.txt`. It writes only ignored artifacts
under `build/mario-update-restoration-20260903/retail/`; it does not update the
production symbol/split files or original-disc directory.

The complete root translation unit was compiled with
`build/compilers/GC/3.0a3/mwcceppc.exe`, the repository's `cflags_game`, and
`VERSION=0`, through `wibo` and the configured `sjiswrap v1.2.2`. The SJIS
wrapper matters: compiler option `-enc SJIS` alone does not transcode UTF-8
source literals. The final objects contain the retail Shift-JIS animation
strings, including `落下`, `基本`, and `水泳陸うちあげ`.

Current `objdiff-cli v3.6.1` results:

| Method | Retail address | Retail/compiled bytes | Match |
| --- | --- | --- | --- |
| `doExtraServices` | `0x802AA048` | 524 / 508 | 92.061066% |
| `checkForceGrounding` | `0x802AA308` | 712 / 712 | 97.764046% |
| `inputStick` | `0x802AC404` | 792 / 792 | 97.060610% |
| `update` | `0x802AD398` | 1412 / 1408 | 99.524080% |
| `actionMain` | `0x802AD91C` | 480 / 480 | 100.000000% |
| `updateGroundInfo` | `0x802ADAFC` | 588 / 588 | 99.965990% |

These are live, relocation-aware fuzzy comparisons, not an assertion that the
entire translation unit or every function is byte-identical. The additional
reference audit confirms all **197 direct calls in original order** across the
six methods. Every referenced constant/string value agrees with retail, as do
the external atan/sin/cos table references. The verifier intentionally compares
constant values rather than compiler-generated local symbol names; the repeated
half-pi load in `inputStick` is a remaining allocation difference. Vtable calls,
field offsets, masks, and branch differences were also reviewed in the aligned
instruction output.

The compiler reports two pre-existing warnings in `MarioActor.hpp`, concerning
the nontrivial vector members of unions at `_468Vec` and `_F2CVec`. No new compile
errors remain. `original-compiler.log` preserves those warnings.

## Corrections proved by the current executable

1. **Ground contact uses the returned bool.** The baseline
   `updateGroundInfo` called `checkGround()` then copied movement bit `_1F` to
   `_1`. Retail calls `checkGround` at `0x802ADB48`, then at `0x802ADB54` executes
   `rlwimi r4,r3,30,1,1` (`5064f042`), inserting the returned `r3` bit into the
   grounded flag. Root now assigns `mMovementStates._1 = checkGround();`.

2. **The historical shadow-floor threshold had the wrong direction.** In
   `update`, the correction is enabled when the projected delta is **greater
   than or equal to 5**, followed by a positive front-vector dot product. Retail
   `0x802AD5E0..0x802AD5E8` is `fcmpo`, `cror eq,gt,eq`, then branch on false
   (`fc010040 4c411382 4082004c`). The historical body used `<= 5.0f`; the
   restored body uses `>= 5.0f`. The referenced float is exactly `40a00000`.

3. **Force grounding must retain the ordered guard.** After `fabs`, retail
   `0x802AA484..0x802AA488` compares against 30 and branches to the epilogue
   unless LT is set (`fc010040 40800124`). This also exits on an unordered
   comparison. Historical `if (abs >= 30) return;` would continue for NaN.
   Root uses `if (!(abs < 30.0f)) return;`, preserving the retail condition.
   The float bits are `41f00000`. No new fallback or finite-value clamp was
   added.

4. **The slip normal is acquired in retail order.** `update` obtains the
   second polygon-normal reference before constructing the `_368` vector copy,
   then subtracts that reference. A named const reference preserves this order
   and keeps the complete direct-call sequence equal to retail. This avoids
   relying on expression evaluation order around `getNormal`.

The complete restored `update` retains the original damage/swim early exits,
wall and ground checks, first-person request handling, `actionMain` dispatch,
area and rail force movement, grounded correction, collision actions, physical
writeback, timers, and extra services. It introduces no PC-only jump handler.

## Required declarations and includes

Only `src/Game/Player/Mario.cpp` and `include/Game/Player/Mario.hpp` changed in
production source.

| Declaration fix | Evidence |
| --- | --- |
| `checkForceGrounding`: `bool` → `void` | Historical definition and retail guard exits have no result contract; caller ignores the result. |
| `updateBinderInfo`: `void` → `bool` | Existing `MarioCollision.cpp:1344` definition; retail `update` consumes `r3` into movement bit `_7`. |
| `checkGround`: `void` → `bool` | Existing `MarioCollision.cpp:1596` definition; returned grounded flag described above. |
| `damageFloorCheck`: `void` → `bool` | Existing `MarioDamage.cpp:193` definition; retail `update` tests the result and performs early writeback/return. |
| `damageWallCheck`: `void` → `bool` | Existing `MarioDamage.cpp:248` definition; retail `update` tests the result before continuing. |
| `isUseFoolSpecialGravity` → `isUseFooSpecialGravity` | Coordinated declaration-only fix for the gravity recovery agent. Existing `MarioSpecial.cpp:94`, retail symbol `0x802F3294`, size `0xBC`. |

`KarikariDirector.hpp` and `JMATrigonometric.hpp` provide the real declarations
used by the recovered methods. Existing member names/layout were sufficient;
no storage/layout changes or fabricated member casts were needed.

There is another pre-existing return-type drift outside the required edits:
`damagePolygonCheck` is declared `void` in `Mario.hpp`, while
`MarioDamage.cpp:286` defines `bool`. This caller discards its result, so it did
not block the current compile. It is reported for the later full damage-source
import rather than broadened here.

## Remaining differences and integration boundary

- `actionMain` has a 100% objdiff result. `updateGroundInfo` differs only in a
  compiler-generated relocation name for the same 170.0f constant.
- `update` retains a four-byte reduction from reusing the loaded movement-word
  register in its grounded/jumping gate. Its control path and calls agree after
  the corrections above.
- `doExtraServices` keeps the existing source. Differences are register
  allocation and equivalent boolean tests (`cntlzw`/`srwi` versus `cmpwi` with
  inverted branches). Actor `_3C0`, the Foo/Teresa exclusions, timers, gravity
  projection, and kill thresholds match the retail references and offsets.
- `checkForceGrounding` retains equivalent early-exit branch layout and local
  vector/register allocation differences. The formerly different unordered
  guard is now corrected.
- `inputStick` retains load/register scheduling differences, a half-pi reload,
  and commuted scalar multiplication operands. It uses the same single-precision
  operations, margin/quarter logic, truncation/mask table access, and real
  atan/sin/cos tables. This does not prove cross-architecture floating-point
  equivalence of the later PC math implementation.

This task does not make original `update` safe to activate on the current PC
Mario construction by itself. The prior
`notes/mario-jump-audit-20260903/README.md` records the full dependency frontier:
actual state objects (`Wall`, `Hang`, `Swim`, and others), animation construction
and update, gravity, water registry queries, collision closure, effects and
other original actor control services. Other agents are handling independent
gravity and animation pieces. Root owns the eventual PC mirror, build gates,
replacing the grounded-only native movement path, and real-disc jump tests.

## Reproduce

Required ignored tools are the versions pinned in `configure.py`. If absent,
fetch them using the repository downloader:

```sh
python3 tools/download_tool.py sjiswrap build/tools/sjiswrap.exe --tag v1.2.2
python3 tools/download_tool.py dtk build/tools/dtk --tag v1.8.3
python3 tools/download_tool.py objdiff-cli build/tools/objdiff-cli --tag v3.6.1
python3 pc-port/notes/mario-update-restoration-20260903/verify-source.py
python3 pc-port/notes/mario-update-restoration-20260903/verify-object.py
```

`verify-source.py` checks the recorded current/historical source-body hashes.
`verify-object.py` compiles the actual root TU with its actual includes, splits
the verified retail DOL without changing its configuration, runs objdiff, checks
the complete direct-call sequence plus data references, and emits:

- `build/mario-update-restoration-20260903/Mario.command.json` and compile log;
- `build/mario-update-restoration-20260903/objdiff.json`, the complete comparison;
- `build/mario-update-restoration-20260903/six-function-comparison.txt`, aligned
  instructions with retail addresses and compiled function offsets;
- `build/mario-update-restoration-20260903/compiler-evidence.json`, copied into
  this note for the current checkpoint.

`source-correspondence.json` records exact source/header and six method hashes,
the historical starting bodies, baseline availability, retail ranges/hashes,
and every intentional adaptation. No whole-game binary match is claimed.
