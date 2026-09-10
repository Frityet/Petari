# Original CollisionParts line query

Recovered the missing `CollisionParts::checkStrikeLine` in `decomp/src/Game/Map/CollisionParts.cpp`, following `decomp/AGENT_DECOMP_GUIDE.md`. The original declaration incorrectly returned void; the retail routine returns its accepted hit count, so the reference header now declares u32. The native header already used u32 and was preserved.

## Retail behavior and proof

Retail address **801771A4–8017738C**, 492 bytes, in `notes/gateway-audit-20260907/restoration/retail/asm/Game/Map/CollisionParts.s`. One full Wii TU compilation succeeded; the recovered function matches **100%**. Exact command and bounded symbol result are in `proof.json`; instruction comparison is in `objdiff.json`. This score applies to this routine only.

The method measures the world offset length, transforms start and end through mInvBaseMatrix, computes the local segment offset, then calls the existing actual KCollisionServer::checkArrow. Its stack arrays each hold64 entries, and the original caller supplies the remaining capacity. It transforms each local intersection back through mBaseMatrix and fills the actual Triangle before calling the optional filter. True means reject. Accepted hits are compacted and receive world distance, world hit position, and the flag byte. The return is the accepted count, rather than the pre-filter KCL count. The original query ordering, per-parts capacity, and callback allocation domain are retained.

## Native integration

The exact recovered body is mirrored into `src/Game/Map/CollisionParts.cpp`. This file remains excluded while the class's existing native extraction in `src/compat/OriginalCollisionPartsCompat.cpp` publishes the routine. Both add only required HitInfo/TriangleFilter includes. Existing native KCollisionServer and Triangle::fillData provide the complete dependency chain; their real decoded KCL/resource ownership is retained, with no extra collision resource or gameplay shadow state.

The active extraction has one explicit portability delta: `u8 flags[64] = {};`. Retail checkArrow's all-hit branch never writes its pFlags array, although CollisionParts copies the corresponding uninitialized stack bytes to HitInfo::_88. The recovered reference keeps that exact behavior for codegen proof; PC initializes these otherwise undefined bytes to zero, matching the pre-existing native line-query policy documented in GameMapCollisionCompat. No edge/face classification is fabricated, and no global KCL algorithm change was made.

One isolated full native provider compilation succeeded using current compilation database flags. `native-proof.json` records that command and the five source/header hashes. The parent owns the combined keeper link and runtime smoke. No root Xmake, broad tests, commits, or other CollisionParts behavior edits were performed here.
