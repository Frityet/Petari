# Upstream sync through 0933a43cd

## Outcome

Merged `upstream/master` at `0933a43cd4e994682d922b61b56d73d3067689bc`
into `pcp-aurora` as merge commit
`2eb21a12e11aef0a3855270a4ac331a4b906e7f8`.

The merge brought in 24 upstream commits and 48 root-only changed paths:
JSystem/JKernel, RVL SDK, root camera decompilation, and small root NPC, Player,
and Ride changes. It changed no `pc-port`, Aurora, or Dolphin file. In
particular, it did not touch the user-dirty `pc-port/src/Game/Screen/SaveIcon.*`
or `pc-port/src/Game/Util/TriggerChecker.*` files.

## Merge audit

Relative to merge base `4afa51b76c8bd30b6640cfda0f739391f5abe9c5`,
only three paths were changed by both branches:

- `configure.py`: local RMGK02/autodetection support and upstream
  `-DMETRO_TRK` plus camera matching flags were disjoint and retained.
- `libs/JSystem/include/JSystem/JGeometry/TMatrix.hpp`: local `element()`
  helpers and upstream rotation helpers were disjoint and retained.
- `src/Game/Camera/CameraDirector.cpp`: the local source reconstruction and
  upstream `mIsRotatingHard` field rename auto-merged cleanly.

A pre-merge `git merge-tree` simulation contained no conflict markers. The
actual ort merge auto-merged all three paths without manual resolution.

## RMGK02 verification

RMGK02 was regenerated explicitly because its config intentionally shares the
RMGK01 symbols/splits files, and upstream changed the camera data splits. The
merged build graph contains both `-DVERSION=1` and `-DMETRO_TRK` where required.

The full linked build and all report objects compiled. The final DOL exactly
matches the extracted RMGK02 executable:

```text
54b71431af0d509097bfdef4ec28617afc487e89
```

The regenerated report now records 64.670616% matched code, 77.68499% fuzzy
match, 14.227636% complete code, and 732/2220 complete units.

## PC-port verification

- Full debug `smg-pc` build passed.
- DemoSheet runtime tests passed 11/11.
- DemoScene runtime tests passed 12/12.
- Aurora-native tests passed 27/27.
- Title, six-slot file select, picturebook, and Gateway handoff route checks
  all passed on separate X displays.
- Their four screenshots are byte-identical to the pre-merge approved
  baseline, so duplicate PNGs were not committed.

No compatibility workaround or PC Game-source edit was added for this sync.
See `verification.log` for compact command evidence and `manifest.json` for
machine-readable results.
