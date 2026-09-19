# Final duplicate-provider removal

The baseline `before-final-owner-cleanup/duplicate-strong-providers.tsv` reports 14 symbols with multiple strong definitions. This source change removes the extra compiled owners without replacing their gameplay behavior.

| Symbols | Removed provider | Retained owner / evidence |
| --- | --- | --- |
| `MR::getRootPane`, `copyPaneRotate`, `createAndAddGroupCtrl` | `src/compat/OriginalLayoutPaneQueries.cpp` | `src/Game/Util/LayoutUtil.cpp`; each body is token-identical to both compatibility copy and decomp donor. |
| `MR::convertPaneLocalPosToScreenPos` | Same file | `src/Game/Screen/LayoutCoreUtil.cpp`; canonical body is token-identical to donor. Deleted copy used `pPane->mGlbMtx.m`, while canonical passes `pPane->mGlbMtx`. `src/nw4r/math/types.h:141–153` supplies the original matrix-to-pointer conversion to the same 3x4 storage. |
| Five `RumbleData` functions | `src/compat/WPadRumbleDataSource.cpp` | `src/Game/System/WPadRumbleData.cpp`; removed file only included that same `.cpp`, creating a second copy of its functions and static pattern table. |
| `J3DModelX::copyExtraMtxBuffer`, `swapDrawBuffer`, `setDynamicDL` | `src/compat/J3DModelXDrawCompat.cpp` | `src/Game/Player/MarioActorDraw.cpp`; all three bodies token-identical to compatibility copies and decomp donor. |
| `MarioAccess::changeAnimationE(const char*,const char*)` | `src/compat/OriginalMarioAnimationAccess.cpp` | `src/Game/Player/MarioAccess.cpp`; body token-identical to compatibility copy and decomp donor. |
| `FORCE_SCALE()` | No body removed; three compiled definitions made `[[maybe_unused]] static` | Narrow linkage correction for decompilation template-instantiation helpers in `CoinHolder.cpp`, `MarioEnforce.cpp`, and `OriginalCollisionPartsCompat.cpp`. Repository search finds definitions only, no calls; baseline executable has no such symbol. Actual gameplay bodies remain unchanged. The excluded canonical `Game/Map/CollisionParts.cpp` is untouched. |

`before-final-owner-cleanup/duplicate-removal-proof.json` retains hashes and exact token-comparison results for the eight wrappers. This is source evidence, not a binary equivalence claim. No new decompilation was performed.

The target uses the `compat/**.cpp` glob, so no target edits are needed. `libsmg-pc-game.a` must be recreated before final audit/link to remove stale members from the four deleted sources. The coordinating build agent owns that rebuild and the post-change provider audit. Runtime startup and Gateway smoke must use the resulting final binary, not the earlier `92a09cc0...` checkpoint.
