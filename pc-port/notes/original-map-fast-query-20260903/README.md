# Original sorted and fast collision queries — 2026-09-03

Recovered three complete MapUtil methods, covering 1,388 retail instruction
bytes, root first. The original Game compiler produces:

| Method | Retail address | Bytes | Objdiff |
| --- | --- | --- | --- |
| getNearPolyOnLineSort | 0x803E2130 | 520 | 99.42308% |
| getCameraPolyFast | 0x803E3870 | 312 | 99.80769% |
| getFirstPolyOnLineBFast | 0x803E39A8 | 556 | 99.71223% |

Every call, branch, field access, constant, arithmetic operation and relocation
matches the retail instruction sequence. Camera fast relocates byte-for-byte.
The sort routine differs only in its source/destination pointer registers over
instructions 97–118. BFast differs only in five disjoint 12-byte stack slots.
`verify-original.py` asserts these exact correspondences, all relocation
operands against the DOL, and constant pool contents. Mutable mSortBuffer and
mSortCount retain their symbol identities; count uses the actual r13 small-data
base 0x806B9620, not the r2 constant pool. DOL SHA-1:
`25c5959534b3c21246c6c7e42021b916b41fb578`.

Sort asks the actual map keeper for its bounded line-hit list, excludes only
the requested sensor, and repeatedly selects the strictly nearest remaining
hit. It copies actual Triangle and HitInfo fields into the original shared
32-entry buffer. Zero hits returns immediately without clearing mSortCount,
as in the original. The comparison starts at 1,000,000 and equal-distance hits
retain encounter order. No extra geometry or null identity is constructed.

Both fast queries traverse 5,000-unit segments, shrink the final segment, and
use the original 0.001 residual threshold. BFast skips water, then rejects a
candidate when a 35-unit normal probe starting 5 units behind it hits map
collision. Its actual keeper query uses the original one-hit capacity. The original
normalization, optional outputs and control flow are retained, including the
unused initial Triangle construction.

`root.patch` contains only these three root methods relative to checkpoint
`da9a51993`. The complete root TU is copied without adaptations to
`build/original-map-fast-query-20260903/staged/Game/Util/MapUtil.cpp`; native Game
compilation passes against the staged collision owners. `native.patch` is the
full delta against production native MapUtil; `native-incremental.patch` is only
this group on top of the previous original-map-query-access staging. Do not
apply both. Native production, shared builds and GPU execution were untouched.

This is source and isolated compile proof, not placed-collision runtime proof.
Actual keeper/part point and area queries, same-host traversal, camera/zone
ownership and scene quiescence remain activation requirements. The original
Binder and Mario path must remain gated until that query and identity graph is
complete. Collision owner application must preserve the current registry host
allocation guards: apply only its named collision ownership include and retire
hook, not an old whole ActorRuntimeRegistry snapshot.
