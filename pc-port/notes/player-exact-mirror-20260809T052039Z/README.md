# Exact Player source mirror boundary

Snapshot: 2026-08-09T05:20:39Z.

## Result

The complete 96-translation-unit constructor-seeded Mario closure is now
source-present under `pc-port/src/Game/Player`. All 96 `.cpp` retail branches
are byte-identical to the current root decompilation sources. The 63 Player
headers required by that closure are also byte-identical.

Before this sync, only `MarioHolder.cpp` and `MarioHolder.hpp` were present and
exact. The other 95 sources and 62 required headers were absent; there were no
divergent Player mirror files to preserve or reconcile.

Source presence does not activate the retail player. `MarioHolder.cpp` remains
the only Player TU compiled into `smg-pc-game`. The other 95 closure sources
have individual `remove_files` entries in `src/Game/xmake.lua`; neither the
Mario nor MarioActor placement creator was enabled. A broad Player removal
followed by an explicit MarioHolder re-add was tested, but xmake omitted the
re-added object from the archive despite listing it in `xmake show`, so the
proven explicit form is retained. The generated
`compile_commands.json` contains exactly one `src/Game/Player` production
entry, `MarioHolder.cpp`.

The precise per-TU state is recorded in `mirror-inclusion.tsv`.

## Regression boundary

`smg-pc-player-source-mirror-tests` checks all 96 source mirrors, all 63 header
mirrors, and the production inclusion policy. It fails if any mirrored file
diverges, if any provider-incomplete TU loses its explicit exclusion, or if
`MarioHolder.cpp` becomes excluded. `MarioActor.cpp`, `MarioActorDraw.cpp`, and
`MarioAnimator.cpp` are the only permitted debug-divergent mirrors. Their
marked `NDEBUG` retail branches are reconstructed by the test and must still
match the root files byte-for-byte.

A second xmake compile-only target was deliberately not added. The frozen
61/96 syntax probe already measures that boundary, and it currently depends on
nine audit-only `revolution/*` forwarding headers. Reusing those note-local
overlays in a production xmake target would make the source boundary less
honest and duplicate the existing probe. The mirror gate is independent of
that provider work.

## Verification

```sh
xmake build -j 12 smg-pc-player-source-mirror-tests
xmake run smg-pc-player-source-mirror-tests
xmake build -j 12 smg-pc-game
```

Observed result:

```text
Player source mirror passed: 96/96 retail source branches exact, 63/63 headers exact, 1 production TU and 95 explicit exclusions
```

The `smg-pc-game` build also passed.
