# System / Util merge repairs

The automatic `-X theirs` merge retained nonconflicting fragments of duplicate fork recoveries next to complete upstream implementations. The first full decomp build confirmed compiler errors. Resolve these semantic collisions by taking the complete upstream files; no nonoverlapping bodies were present in the residual diffs of these five files.

- `src/Game/System/DrawSyncManager.cpp`: removes duplicate static `sInstance`, `drawSyncCallback`, `threadFunc`, and constructor. The stale fork constructor/body also referenced fields renamed in the upstream header. Use upstream Fifo accessors and current member names.
- `src/Game/System/WPadAcceleration.cpp`: removes duplicate constructor and `getAcceleration` plus obsolete `sLimitSwingDirection`; upstream has the complete matching implementation.
- `src/Game/System/GameEventValueChecker.cpp`: use upstream `readU16` sequence; the mixed merge retained fork reads referring to `readHash`/`readValue` locals removed upstream.
- `src/Game/Util/EventUtil.cpp`: removes duplicate `starId` local produced by two independently inserted declarations.
- `src/Game/Util/MathUtil.cpp`: removes fork `JMASqrt` body now implemented upstream later in the file.

The full build is coordinated by the parent agent. Initial errors are captured in `build-first.log`; no concurrent ninja processes were started by this agent.
