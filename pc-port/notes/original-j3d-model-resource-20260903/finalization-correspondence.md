# Original hierarchy and model finalization correspondence

The native owner assembles retained typed components, then invokes the actual original hierarchy/finalization code. This note distinguishes that source correspondence from the owner's resource validation and lifetime policy.

## Reproducible proof

From the repository root:

```sh
python3 pc-port/notes/original-j3d-model-resource-20260903/verify-finalization.py
```

The script writes `finalization-source-evidence.json` and keeps compiler commands, objects, split output, and objdiff under ignored `build/original-j3d-model-finalization-20260903/`. It tokenizes C++ while preserving string/character literals, identifiers, operators, and constants; only comments and whitespace disappear.

All six comparisons pass:

| Code | Tokens | Allowed adaptation |
| --- | --- | --- |
| `J3DJointTree::makeHierarchy` | 251 | None; native body equals root |
| Hierarchy command enum | 26 | None |
| `J3DModelLoader::setupBBoardInfo` | 219 | Function name changes; its two original member inputs become explicit parameters |
| `J3DJoint::addMesh` | 21 | None; existing original inline body |
| Ordinary model finalization | 89 | Explicit model reference and billboard inputs; original return handled by owner |
| Binary model finalization | 51 | Same explicit inputs; selects the original binary suffix |

`finalize_j3d_model` is checked separately with its binary selector true and false. Both expansions must equal the corresponding root `load`/`loadBinaryDisplayList` tail, preserving hierarchy creation, shape initialization, VCD/VAT sorting, important matrix-index selection, billboard setup, and either binary `indexToPtr` or ordinary `0x100` → shape `0x200` propagation. No alternate hierarchy or billboard calculation is accepted by this comparison.

## Original compiler and binary evidence

The script recompiles root `J3DJointTree.cpp` with GC3.0a3, the Shift-JIS wrapper, and the repository's JSystem flags. The input is the verified RMGK01 rev0 DOL with SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`.

`makeHierarchy` at `0x804316C4` scores **95.73034%**: 356 retail bytes versus 348 compiled bytes. The sole instruction difference is the compiler emitting the existing `addMesh` body out of line. The verifier checks that complete six-instruction helper, its actual caller argument identities, and the exact five-instruction retail inline expansion at `0x804317DC`. Expanding only that proven call produces equality for all **89** retail instructions, with branches rebased to their corresponding instruction indices.

Jump-table references are not treated as strings or reduced to a constant prefix. All **19** relocation targets match, and their full absolute words match the original 76-byte table at `0x805E99E8`.

The two newly restored SDK dispatchers have independent 100% / relocated-byte evidence in `../original-sdk-model-dispatch-20260903/`. Native SDK entrypoints deliberately reach a bounded registered resource owner; this token proof does not claim those host ownership entrypoints are the original PPC dispatcher implementation.

## Focused owner review and fixes

The parent addressed the three issues from the initial read-only review:

- `CommandScope` encloses ordered patch validation and all original finalization, retaining the cooperative CPU gate with a nested scheduler hold. It restores the caller's GD pointer and interrupt level before releasing that hold. This covers both original shape-command generation and model `indexToPtr`. The one-time static interrupt snapshot in original `indexToPtr` is genuine retail behavior, independently observed at `0x80431B04`–`0x80431B30`; the Game/SDK algorithm remains unchanged.
- Loaded-data retirement clears `j3dSys`'s texture selection only when it still selects the retiring owner's texture. Destroying an older owner therefore cannot clear a newer model's selected texture. Destruction occurs under the same command-state boundary before the texture backing is released.
- Display-list prevalidation merges overlapping actual command intervals, copies them into shared scratch regions, and calls original `loadTexNo` in material order. Later aliased views see the earlier real patch bytes. Preview GD, texture, and texture-coordinate scale state is restored on success and exception. Scratch storage now has explicit 32-byte alignment and retains each list's relative offset; unaligned or too-short original views reject before `GDInitGDLObj`.

The subsequent focused read-only review found no further defect in those changes. Component destruction order, retained source aliases, and accepted hierarchy parent/current/prepend behavior also remained consistent with their contracts. Parent-reported native regressions cover actual Mario resources under all three material flag modes, first-disabled/later-enabled OS state, current GD restoration, A/B texture retirement, and rejection of an unsafe aliased mutable patch stream. This reviewer did not run native tests or shared builds.

This proof covers imported source correspondence and the original hierarchy's compiled instructions. It does not certify all material decoding, resource error paths, graphics execution, the full ResourceHolder migration, or playable jumping.
