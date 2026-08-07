# FixedPosition real-or-absent audit

Timestamp: 2026-08-07 03:25:44 UTC

## Outcome

The PC compatibility implementation no longer ignores a requested FixedPosition
resource and silently substitutes the host actor's base matrix with zero offsets.

- `pc-port/src/Game/Util/FixedPosition.cpp` is an exact copy of the regular
  decompiled `src/Game/Util/FixedPosition.cpp` (matching SHA-256 recorded in
  `verification.log`). It is kept as source-parity evidence and excluded from the
  host build because the host-adapted `LiveActor`/resource ABI differs.
- `pc-port/src/Game/Util/FixedPosition.hpp` remains byte-identical to the regular
  decompiled header.
- The compatibility layer now looks up the requested model RARC and requested
  `<name>.bcsv`, parses row zero, and consumes the actual `JointName`,
  `TransX/Y/Z`, and `RotateX/Y/Z` fields.
- A missing resource, missing runtime/model archive, or currently unavailable
  named-joint matrix is rejected explicitly. No actor-base matrix is substituted
  for a requested joint.
- Actor-relative and direct-matrix FixedPosition construction remain supported
  and retain the original matrix calculation behavior.
- The implementation contains no route, stage, actor-name, or title-sequence
  special case.

## Retail/decomp evidence

The regular decompiled constructor in `src/Game/Util/FixedPosition.cpp` requests
`%s.bcsv`, reads row zero's `JointName`, then reads vectors with the prefixes
`Trans` and `Rotate`. The RMGK02 oracle confirms those literal names in
`build/RMGK02/asm/Game/Util/FixedPosition.s`.

RMGK02 `MR::getCsvDataVec` formats each prefix with `%sX`, `%sY`, and `%sZ` before
calling `getCsvDataF32`; those literals are emitted at `0x806B2680` through
`0x806B2688`. Consequently the compatibility reader uses the real field names
`TransX/Y/Z` and `RotateX/Y/Z`. Like the original constructor's zero-initialized
vectors, an individual absent float field retains zero; this is retail behavior,
not a fallback for an absent resource.

The host renderer can parse J3D joint names, but the current PC actor API does not
expose the live animated joint matrix required by this constructor. Until that
real path exists, a requested `JointName` makes construction fail explicitly.

## Focused coverage

`FixedPositionRealOrAbsentTests.cpp` builds real big-endian BCSV bytes inside a
minimal RARC and verifies:

1. Requested translation and rotation values are returned exactly rather than
   invented as zero offsets.
2. A requested joint name is preserved.
3. A missing BCSV remains absent.
4. Resource construction without real backing data and unavailable named-joint
   construction both fail explicitly.
5. The supported actor-relative path still produces the actual world transform.

The focused target passed 5/5. Combined integration and title/file-select/
picturebook/Gateway route verification is intentionally recorded by the parent
integration audit rather than duplicated here.
