# JointController RMGK02 decomp evidence

Captured at `2026-08-08T20:24:19Z` from branch `pcp-aurora`.

The root decompilation unit `main/Game/Util/JointController` increased from
`72/684` matched code bytes and `4/10` matched functions to a complete match:

- code: `684/684` (`100%`)
- data: `16/16` (`100%`)
- functions: `10/10` (`100%`)
- fuzzy match: `100%`

The recovered source supplies the real pre-child and post-child matrix callback
pipeline, both joint-parameter overloads, and the compiler-emitted
`J3DModel::getAnmMtx(int)`. `JointControllerInfo` now carries the real
`JointController*` and `J3DJoint*` pair instead of a partial stand-in for the
joint layout.

Evidence files:

- `focused-objdiff.txt`: complete unit and per-function metrics.
- `rmgk02-build.log`: full-build and canonical DOL SHA-1 verification.
- `aurora-contract.md`: recovered callback semantics and the remaining honest
  host-side implementation boundary.
- `pc-sync-verification.log`: byte-identical PC mirror and host build evidence.

The exact header and implementation were subsequently mirrored into
`pc-port/src/Game/Util`. The implementation remains explicitly excluded from
`smg-pc-game`, so the existing compatibility-layer rejection continues to
represent the missing renderer hook honestly. The mirror regression test now
checks both files byte-for-byte.

The source changes, PC mirrors, and this evidence bundle were deliberately left
unstaged and uncommitted for integration by the primary agent.
