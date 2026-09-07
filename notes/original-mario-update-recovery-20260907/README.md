# Original Mario update recovery, 2026-09-07

Restored the missing `Mario::update()` into the decompilation reference, following
`decomp/AGENT_DECOMP_GUIDE.md`. The complete recovered body comes from root commit
`e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4:src/Game/Player/Mario.cpp`. Its method hash
matches the earlier documented recovery exactly; the current proof independently
recompiles both the restored translation unit and the pre-change baseline using
the configured Wii compiler and current headers.

The original update performs collision and damage checks, guarded camera-mode
requests, stick input, state-machine action, floor correction, external forces,
grounding, step/bump/warp checks, and final physical-vector/timer writeback in the
retail order. It retains the original early returns, flag manipulation, constants,
and diagnostic timing calls. The only accompanying source edit is the include
for the original Karikari count query.

Fresh validation:

- Complete restored and baseline Wii translation units compile successfully.
- `update__5MarioFv` matches retail **99.52408%**: retail 1,412 bytes at
  `0x802AD398`, candidate 1,408 bytes.
- All 85 direct call relocations match in order; referenced constant/string
  payloads and external data relocations match.
- No existing function's match score regresses against the fresh baseline.
- The actual reference DOL SHA-1 is
  `25c5959534b3c21246c6c7e42021b916b41fb578`; the retail function bytes have SHA-256
  `e1dfec32d31b921a251f531208db48d702378a4ca02dac3affe0d09a975a8ef9`.

`compile-results.json` records complete commands and results;
`function-proof.json` records scores, call order, relocations and payloads;
`source-proof.json` records provenance and source hashes. Baseline/restored object
comparisons and the recovered method are retained alongside this note.

**Native Mario.cpp is unchanged.** The original constructor and state ownership
graph must be made real before this update can replace the native stand/walk
implementation. This checkpoint establishes a verified reference body, not
working original native movement or camera behavior. The existing owner inventory
is in `notes/original-player-animation-20260907/state-owner-source-inventory.json`.

Only decomp source path: `src/Game/Player/Mario.cpp`. No header, native build-list,
Git index or commit changes were made by this lane for this recovery.
