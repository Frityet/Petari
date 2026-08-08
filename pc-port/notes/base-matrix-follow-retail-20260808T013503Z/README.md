# Base-matrix follower retail recovery

## Outcome

The root decompilation now contains the complete retail implementation of
`BaseMatrixFollowTargetHolder.cpp`. This is source recovery in `src/Game`, not a
PC compatibility workaround.

Recovered behavior:

- calculate the follower matrix as `host base matrix * inverse target placement`;
- keep separate target and follower vectors in their retail object-layout order;
- create unresolved link targets when followers register;
- resolve target placement links by all three `JMapLinkInfo` fields;
- bind gravity-follow hosts after placement;
- update only followers that have a resolved live actor;
- expose the authentic scene-object registration helpers; and
- define the retail four-byte no-op base `BaseMatrixFollower::update` virtual.

The header's previous vector order was reversed. Retail disassembly and the
constructor/destructor accesses establish `mTargets` at `0xC` and `mFollowers`
at `0x18`; the recovered header now reflects that layout.

## Evidence

Reference code was recovered from the RMGK02 `main.elf` functions at
`0x80400B40` through `0x80401418`, using the symbols in
`config/RMGK01/symbols.txt` (RMGK02 intentionally shares this symbol/split
configuration).

Verification completed on 2026-08-08 UTC:

```text
ninja -j 12
build/RMGK02/main.dol: OK
```

A fresh `objdiff-cli report generate` comparison for
`src/Game/Util/BaseMatrixFollowTargetHolder.cpp` reports:

```text
fuzzy match:       100.0%
matched code:      2264 / 2264 bytes
matched data:      56 / 56 bytes
matched functions: 25 / 25
```

This includes exact matches for both previously difficult functions,
`BaseMatrixFollowTarget::getHostBaseMtx` and
`BaseMatrixFollowTargetHolder::findFollowTarget(const JMapLinkInfo*)`.

## Files

- `include/Game/Util/BaseMatrixFollowTargetHolder.hpp`
- `src/Game/Util/BaseMatrixFollowTargetHolder.cpp`
