# RMGK02 decompilation status — 2026-08-09 01:49 UTC

This snapshot was regenerated from the current root decompilation with:

```sh
ninja -j12
build/tools/dtk shasum -c config/RMGK02/build.sha1
```

The reconstructed DOL passed the canonical SHA check:

```text
build/RMGK02/main.dol: OK
```

The authoritative machine-readable report is `build/RMGK02/report.json`.

## Current totals

| Scope | Code coverage | Function coverage | Fuzzy similarity | Complete code | Complete units |
| --- | ---: | ---: | ---: | ---: | ---: |
| All | 65.01% (3,519,056 / 5,413,352 bytes) | 80.24% (33,728 / 42,036) | 78.12% | 14.25% | 734 / 2,219 |
| Game | 62.19% (2,579,540 / 4,147,644 bytes) | 79.32% (28,334 / 35,720) | 74.63% | 13.30% | 532 / 1,605 |
| JSystem | 64.01% | 82.59% | 85.66% | 7.11% | 32 / 169 |
| SDK | 76.12% | 86.36% | 90.93% | 18.05% | 76 / 279 |
| NW4R | 70.48% | 85.71% | 87.18% | 19.41% | 17 / 29 |
| RVLFaceLib | 100.00% | 100.00% | 100.00% | 17.50% | 5 / 13 |
| MSL_C | 100.00% | 100.00% | 100.00% | 40.90% | 44 / 61 |
| MetroTRK | 100.00% | 100.00% | 100.00% | 100.00% | 28 / 28 |

`Code coverage` is the fraction of target code bytes represented by matched
functions. `Fuzzy similarity` describes similarity of emitted/matched code and
must not be read as whole-program coverage. `Complete` is the stricter
byte-perfect/fully linked measure used by the progress tool.

Data coverage is 30.67% overall and 43.69% for Game.

## Player/Mario frontier

The retail `Game/Player` module contains 107 target translation units. The
current source tree contains 78 of them and is still missing 29 units. Across
the full 107-unit target:

- code coverage: 28.17% (132,692 / 470,948 bytes);
- function coverage: 53.33% (865 / 1,622 functions);
- fully complete units: 10 / 107.

This explains why the overall decomp percentage is much higher than the
current Mario-spawn readiness: the player module is substantially behind the
rest of Game, and `Mario::Mario` directly constructs several of the missing
state units.
