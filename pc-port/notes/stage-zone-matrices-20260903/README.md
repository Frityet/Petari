# Shared stage placement matrix access

Original `Camera::setZoneMtx` needs `MR::getZonePlacementMtx(s32)`. Other
recovered Game code uses the iterator overload to identify the exact holder
which owns a placement row. Both entry points now resolve real retained stage
holder matrices through a common native lifetime binding.

## Original behavior and recovery

The supplied RMGK01 DOL, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`, establishes:

- `MR::getZonePlacementMtx(const JMapInfoIter&)` at `0x803f7e6c`, size `0x38`,
  calls `MR::getStageDataHolder`, then `findPlacedStageDataHolder`, and returns
  the holder's `mPlacementMtx` at offset `0xac`.
- `MR::getZonePlacementMtx(s32)` at `0x803f7ea4`, size `0x38`, calls the same
  root-holder accessor, then `getStageDataHolderFromZoneId`, and returns the
  same matrix field.
- The const holder lookup at `0x80347ac0`, size `0x48`, returns the root for
  zone ID zero. Otherwise it scans only immediate children, in retained
  `mStageDataArray` order, and returns the first matching zone ID. It does not
  recursively search or collapse repeated zone occurrences.
- Existing recovered `findPlacedStageDataHolder` identifies the holder by the
  row's original data-address range and then recursively searches children.

The two getters were recovered first in root `src/Game/Util/SceneUtil.cpp`,
then that file was copied byte-for-byte to the PC tree. The existing PC build
excludes the full SceneUtil translation unit; the native providers live in
`src/compat/StageZoneMatrixRegistry.cpp`. Original camera source is unchanged.

Local `get_zone_placement_mtx.*` and `stageholder_from_zone_id.*` disassemblies
remain ignored under `build/compat-math-oracle/`. No game binary is staged.

## Native data and ownership

`StagePlacementResolver` already computes every `StageHolderOccurrence`,
including repeated siblings, ancestry, creation order, empty holders, and
composed placement matrices. It previously discarded that vector after
loading tables. `StageAuthoredData::resolve` now retains it alongside the
existing tables; existing table-only resolver callers may omit the holder
output. No resource is reloaded to reconstruct a camera-specific mapping.

`StageZoneMatrixRegistry` owns one stable `TPos3f` per retained occurrence.
Integer lookup follows the original root/immediate-child order, including
first occurrence semantics for repeated zone IDs and matrices for empty
holders.

PC `JMapInfo` already shares its `mData` compatibility owner across copies.
The registry maps each retained table's data-owner identity to its holder
occurrence. Copied placement, start, child, and rail rows preserve that
identity, even after local position overrides, so iterator lookup selects the
exact holder without adding Game metadata or guessing from zone ID. Nested
holder tables remain accessible to iterator lookup as in the original.

`StageZoneMatrixBinding` publishes the registry for an active stage lifetime,
restores the previous binding after nested scopes, and rejects destruction
out of order. Both `StageHostScene` and `GatewayDemoScene` create it immediately
after authored-data resolution and retain it through actor teardown. Matrix
lookup outside a stage, unknown IDs, invalid rows, and foreign data owners
fail explicitly. Stages with no placement catalog may still exist, but their
matrix lookups fail rather than returning a synthetic identity.

## Verification

`StageZoneMatrixRegistryTests.cpp` covers first-occurrence integer lookup,
distinct repeated-zone iterator lookup and copied data ownership, an empty
child holder, a nested holder available only through the iterator API,
unknown/foreign/invalid row rejection, shared mutable matrix addresses,
nested binding restoration, and final withdrawal on teardown.

The changed SceneUtil root/PC pair is byte-identical. The existing Game source
mirror test binary passed, and `git diff --check` passed for owned files.
The target `smg-pc-stage-zone-matrix-tests` is wired in `tests/xmake.lua`.
Its Apple Silicon debug build and runtime checks passed on 2026-09-03.
The real-disc Gateway stand/walk/release run also passed with the new stage
binding active (14,521 collision triangles, player recreation and teardown).

The preexisting root nonconst `StageDataHolder::getStageDataHolderFromZoneId`
wrapper forwarded without casting `this` to const, which selected itself in
C++. Binary `0x80347b08` tail-calls the const implementation. The root wrapper
now uses the explicit const pointer cast to select that original function.
There is no PC StageDataHolder.cpp copy yet; the native registry follows the
verified const lookup behavior. Full StageDataHolder compilation remains
separate work.
