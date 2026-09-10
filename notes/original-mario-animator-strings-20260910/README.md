# Original MarioAnimator update string recovery — 2026-09-10

The crash after correcting Xanime automatic default return came from wrong recovered string operands in `MarioAnimator::update`. This recovery replaces only its 19 string literals with their actual retail values, first in `decomp/src/Game/Player/MarioAnimator.cpp`, then in the corresponding port function. All non-string tokens and all other functions are unchanged. The decomp guide was read before the recovery; no native null bypass or alternate animation implementation was introduced.

## Live causal proof

The exact optimized debug binary `b1faf11e7e8d105825d855933b7ebbc02530fda08e646f0db05c9d73d87b539f` reaches StageInA, then falling (`落下`), then the default (`基本`) at tick 122. At tick 123 the current source calls `stopAnimation("基本", "待機")` from MarioAnimator::update. The default-name lookup for `待機` returns nullptr: that name is absent from MarioAnimatorData. Original stopAnimation then installs the null default as the current animation, causing the observed dereference in XanimePlayer::updateAfterMovement. LLDB captured the complete stack and original player state (current and default both nullptr, previous valid, controller still a valid 60-frame loop) in `null-current-crash-lldb.log`. The debugger process was killed after inspection.

## Exact retail values

RMGK01 `MarioAnimator::update`, `0x802CB580`, length 1804 bytes, sets r31 to `0x805CAC68`. The failing call at `0x802CB808` actually passes pointers `0x805C3F06` and `0x805C3F11`. Their Shift-JIS bytes decode to `崖ふんばり` and `落下`, respectively. These strings live in the adjacent MarioAnimationEfx object's `.data`, starting at `0x805C3DE0`, rather than the MarioAnimator object's own section; that distinction is essential when resolving the original pointers.

The same bounded function contains 19 concrete wrong string operands. The recovered names include `しゃがみ基本`, `基本`, `崖ふんばり`, `落下`, `幅とび`, the original BAS names `SquatWalk`, `BeeCreepWalk`, `Walk`, and the original ice, wall, landing, effort-running, and slider animation names. `string-recovery.json` records every source line, before/after literal, referencing retail instruction address, string address, and Shift-JIS byte sequence. `retail-update-string-operands.json` records the exact object owning each string. The complete original function assembly is retained.

## Validation and scope

Both baseline and recovered complete translation units compile with Metrowerks under the existing RMGK01 include/compiler configuration, exit 0. The recovered native translation unit also compiles with the current toolchain, exit 0. Commands, source hashes, logs, and object comparisons are retained here. The bounded function's objdiff score is 93.1153% both before and after (1804 retail bytes, 1744 candidate bytes). The unchanged score does not establish the old string values were correct; direct retail address/byte recovery is the decisive evidence for this change. Remaining preexisting code-generation differences were not expanded into this recovery.

A token-level check confirmed that replacing string literals with a common sentinel yields byte-identical old/new function text. The port receives the exact recovered function while retaining its existing separate construction-lifetime include/hook outside this function. Native linked tests, further live runtime, and the root checkpoint are parent-owned. Decomp publication followed the other agent’s disjoint Mario.cpp checkpoint. The exact one-file recovery is committed and pushed with codex author/committer as `b2e28ab8691d9de9986d75e09ba068ff695bf03b`; the remote origin/pcp-decomp SHA was verified equal.
