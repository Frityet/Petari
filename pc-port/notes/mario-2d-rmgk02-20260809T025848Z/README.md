# Mario2D RMGK02 recovery

Timestamp: `2026-08-09T02:58:48Z`

This lane reconstructs the root `Game/Player/Mario2D.cpp` translation unit. It
adds no PC activation, compatibility behavior, factory route, debug code, or
shared-header change.

## Recovered behavior

- `Mario::stick2DadjustGround` rebuilds the retail 2D basis from `_F4`, projects
  Mario's ground direction onto the 2D plane, chooses the 2D facing mode from
  its basis dot product, and preserves the retail stick-direction latch.
- The latch is cleared for stopped movement or a direction change beyond `0.3`
  radians. While retained, the input vector is rotated by the signed horizontal
  difference between the latched and current plane directions.
- New latches are accepted only inside the retail 30-to-150-degree camera-space
  window. Rejected input clears both stick axes and the stick magnitude.
- `Mario::calcDir2D` removes the 2D-plane normal from both camera axes,
  normalizes them with the retail zero behavior, and forms the requested world
  direction as the weighted sum of those axes.
- The retail `NrvMarioActor` static-instance definitions reproduce this unit's
  generated constructor exactly.

## Fidelity

| Section or function | Retail size | Match |
| --- | ---: | ---: |
| `.text` | `0x3F0` / 1,008 bytes | 95.54365% |
| `stick2DadjustGround` | `0x2B0` / 688 bytes | 100% |
| `calcDir2D` | `0xD4` / 212 bytes | 78.81132% |
| generated static initializer | `0x6C` / 108 bytes | 100% |
| `.ctors` | 4 bytes | 100% |
| `.sdata2` | 40 bytes | 100% |

The remaining `calcDir2D` mismatch is code-generation shape, not missing
behavior. Through the second scaled-vector construction, the reconstructed
instructions match the retail call and stack sequence. Retail then calls the
out-of-line `TVec3::add`, while the current reconstructed shared JGeometry
header makes MWCC inline that same addition into 20 extra bytes. Repository
history confirms that this exact overload carried `NO_INLINE` before commit
`462262d0e` removed it during a general TVec helper cleanup. The source keeps
the ordinary `pOut->add(vertical)` expression rather than adding a
Mario-specific symbol or compiler workaround outside this unit; restoring a
global JGeometry code-generation contract is outside this Player-only lane.

## ABI and ownership

The existing `Mario.hpp` declarations already prove both method signatures and
all referenced field offsets. No dedicated header or shared declaration change
was necessary. This lane owns only:

- `src/Game/Player/Mario2D.cpp`
- this ignored evidence directory

See `verification.log` for command-level evidence.
