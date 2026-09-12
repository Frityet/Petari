# Upstream merge into pcp-decomp — 2026-09-12

User request: merge upstream into `pcp-decomp`. This updates the reference submodule; it does not automatically import upstream source changes into the native port.

## Inputs and preservation

- Branch: `decomp/` → `pcp-decomp`, originally `ad4f2b1132248520f4b3c00c03a39c966328bcfe`.
- Fresh upstream: `SMGCommunity/Petari` `upstream/master` at `cae223c325948dd50c8711b13ccf90fa72ff2c6a` (`Related to Kameck (#2024)`).
- Merge base: `d1ae0a05cc023d52ecdcbc7731c8c79f0cb84dc6`; 34 upstream-only commits.
- The unfinished LayoutUtil/LayoutManager recovery was saved before merging. `before.json` records exact file hashes; `stash.json` records the specific stash for restoration after the merge commit. Backup source copies are local evidence.
- Native compatibility work and the user's existing staged document deletions stay outside this merge.

## Resolution policy

Retain recovered functions absent upstream, incorporate upstream additions, and resolve competing behavior using the original binary. Mario core, actor, and state cohorts have separate inventories and decision records. No blanket choice of one side is appropriate for these overlapping recoveries.

The small root-owned resolutions are in `root-conflict-decisions.json`: retain the additional area utility declaration, retain both J2DGrafContext overloads, and accept equivalent upstream Wii mail size arithmetic.

## Validation

- Upstream configuration regenerated successfully with `--non-matching`. Its missing MetroTRK `exception.s` configuration warning remains visible in `configure.log`.
- Original-compiler builds of the resolved NWC24Function and J2DGrafContext passed (`root-objects.log`).
- All 82 changed JSystem/MSL source targets passed the original compiler; 81 rebuilt in that invocation because J2DGrafContext was already current (`sdk-result.json`, `sdk-objects.log`, `sdk-targets.json`).
- All 27 changed non-Player Game source targets passed (`game-nonplayer-result.json`).
- All 117 Player/recent-recovery targets passed, comprising all 107 configured Player sources and ten recent talk/text/event recovery sources (`player-recovery-final-result.json`). The initial aggregate caught four calls to a removed local collision helper; these now use its exact SDK expression and the final repeat passes.
- The union contains **226 distinct targets**, covering every changed C/C++ source. `validated-tree.json` records source hashes for the committed merge tree.
- No unresolved index entries, conflict markers, or unstaged reference edits remained before committing. The pending recovery files were subsequently restored byte for byte; `restoration.json` verifies their original hashes, and the temporary stash was removed.

Existing warnings remain: nontrivial union members in MarioActor, the MetroTRK configuration warning, and whitespace already present in the imported upstream files. The original reconstructed per-file nerve initializer blocks also produce duplicate strong definitions in raw Player objects; that issue predates this merge. This pass validates source compilation, not a final reference executable link, a PC gameplay test, or Gateway completion.

## Published result

Merge commit **`a3f7df5062ee9bd96a0ec9104a99aa320afbc193`** is pushed to `origin/pcp-decomp`; the remote SHA was verified. Its parents are the two input heads above. The resulting merge changes 276 files relative to the previous `pcp-decomp` head. `publication.json` records the exact identities.

The parent repository updates only the decompilation gitlink and these notes for this checkpoint. Existing native compatibility work remains available for continuation.

Readable cohort reviews are in `mario-actor.md`, `player-states.md`, and `player-core.md`. `evidence.tar.gz` contains compiler receipts, source/method inventories, retail decision records, and restoration/publication checks; it excludes compiled binaries and saved source-stage scratch copies.
