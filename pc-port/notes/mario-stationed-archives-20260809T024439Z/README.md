# Exact Mario stationed-archive foundation

Timestamp: `2026-08-09T02:44:39Z`

## Outcome

- Added byte-identical PC mirrors of the retail `StationedFileInfo` header and
  table implementation.
- Extended the generalized `ResourceHolderService` with
  `create_and_add_stationed(load_type)`. It filters the exact retail table and
  resolves each archive through the existing DVD/RARC owner; it contains no
  Mario, stage, or route-specific archive list.
- The six Mario rows are selected by retail load type `2`, in retail order:
  `MarioAnime`, `BoneMario`, `Mario`, `MarioFace`, `MarioShadow`, and
  `MarioTornado`.
- Repeated loads reuse the same scene/runtime-owned `ResourceHolder` objects.
- Unknown load types return an empty set and do not fabricate resources.
- This is a prerequisite only. It does not register `Mario`/`MarioActor`,
  eagerly load the full non-player stationed set, or activate a partial Player
  runtime. The same table-driven API can resolve any other retail load type
  when its real lifecycle reaches that phase.

## Exact-source hashes

```text
68d5369f682004e8ebb5b1746b96ef9b6cd522a4c4e87478438011b56a661fd0  StationedFileInfo.cpp
50bfcd24d4c6cdf04bf43a48f17527178091b40294987c414880d338499813e5  StationedFileInfo.hpp
```

Root and PC copies have the same corresponding hash and pass `cmp`.

## Verification

```text
xmake build -y smg-pc-stationed-archive-real-or-absent-tests  PASS
SMGPC_REAL_DISC=/workspaces/pcport/RMGK01.iso \
  xmake run smg-pc-stationed-archive-real-or-absent-tests     PASS 2/2
xmake build -y smg-pc-game-source-mirror-tests               PASS
xmake run smg-pc-game-source-mirror-tests                    PASS
```

The real-disc test opens the actual RMGK01 image, parses all six selected RARC
archives, proves their file tables are nonempty, and proves stable ownership on
a repeated table-driven request.
