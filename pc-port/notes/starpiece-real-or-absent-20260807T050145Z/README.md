# StarPiece: exact source or absent

## Outcome

- `pc-port/src/Game/MapObj/StarPiece.cpp` and `.hpp` are byte-identical to the root decompilation copies.
- The exact `StarPiece.cpp` is compiled through `src/compat/StarPieceCompat.cpp`; the wrapper contains declarations and the host base-matrix type adapter only, not alternate StarPiece behavior.
- The former 233-line synthetic StarPiece implementation is gone. This removes its guessed initial color, invented floating/rail/fall movement, no-op actor callbacks, reduced message handling, and process-global declaration map.
- `StarPieceGroup` and `StarPieceFlow` are deliberately absent from the PC placement factory. Their exact runtime requires the real `StarPieceDirector` plus player, effect, star-pointer, shadow, collision-triangle, and rail closure that is not linked yet. No partial object is constructed in its place.
- `MapObj/StarPieceGroup.cpp` is excluded while those factory entries are absent. This avoids presenting an unsupported placement as functional.
- StarPiece-specific cleanup/count hooks were removed from the general actor registry because the synthetic declaration state no longer exists.

## Source boundary

The PC `Game/LiveActor/Nerve.hpp` is now byte-identical to the root header, restoring the real `NEW_NERVE_ONEND` surface used by StarPiece. The wrapper contains two local compile adapters: it qualifies the member pointer accepted by modern C++, and selects the original unsigned gravity overload in the presence of the host compatibility overload. Neither adapter changes the root or PC Game source.

## General compatibility work

- Added quaternion axis-to-axis rotation and vector rotation to the host JGeometry compatibility type.
- Added axis-angle matrix rotation to the host JGeometry compatibility type.
- Aurora now provides the original `revolution/gx/GXEnum.h` and `GXStruct.h` include paths as generalized forwarders to its Dolphin-compatible GX headers.
- Aurora commit `375896babb9376dcd13fbd6470740548ab8d3a15` was pushed to `main`.

## Evidence

- `cmp` returned zero for StarPiece `.cpp`, StarPiece `.hpp`, Nerve `.hpp`, PowerStar `.hpp`, and StarPieceDirector `.hpp` against their root decompilation copies.
- `xmake build smg-pc-game`: passed; this compiled `src/compat/StarPieceCompat.cpp` containing the exact source.
- `xmake build smg-pc`: passed.
- `xmake run smg-pc-aurora-native-tests`: 26/26 passed, including `StarPieceGroup factory absent without real director`.
- `git diff --check` on the changed StarPiece/source-boundary paths: passed.

This does not claim working StarPiece gameplay. The source is real; creation is absent until its real dependencies are present.
