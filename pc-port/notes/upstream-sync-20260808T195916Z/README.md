# Upstream sync 2026-08-08

## Scope

Fetched `upstream/master` at `1dbda8c89` and merged the 12 commits after the
shared base `0933a43cd`. The upstream wave adds the 97%-class PlayerUtil work,
100%-class SystemUtil work, MultiEventCamera, SpinDriverCamera improvements,
NWC24 parsing/configuration, and RSO linker progress.

## Conflict resolution

Four files conflicted because this branch had independently recovered the same
retail units:

- `Mario.hpp`: retained the local `bool` binder/ground declarations because
  the linked local `MarioCollision.cpp` definitions return `bool`; upstream
  does not yet contain that source unit.
- `MarioAccess.hpp`: retained the otherwise identical local declaration set
  with the matrix/type includes required by the local source closure.
- `SequenceUtil.cpp`: retained the local implementation, which the fresh
  RMGK02 report proves is 100% code/data/functions, instead of replacing it
  with the older upstream add/add candidate.
- `PlayerUtil.cpp`: retained the local functional implementation and
  RushEndInfo field semantics, then adopted upstream's corrected const return
  signatures for player gravity and ground normal. This raised the fresh fuzzy
  score from 96.6655% to 97.6873%.

The automatic SystemUtil merge placed both independently recovered restart-ID
accessor pairs into the file. The duplicate pair was removed in favor of the
new upstream helper-based implementation. The resulting SystemUtil is 100%.

No PC-specific Game workaround was added during the merge. Exact PC mirror
updates and compatibility adjustments are intentionally handled as the next
port integration step rather than editing decompiled Game sources.

## Safety

The user-owned `pc-port/src/Game/Screen/SaveIcon.*` and
`pc-port/src/Game/Util/TriggerChecker.*` working-tree edits remained unstaged
and untouched. Disc images, build outputs, and other unrelated untracked files
were not added.

See `verification.log` for exact hashes, commands, and RMGK02 measurements.
