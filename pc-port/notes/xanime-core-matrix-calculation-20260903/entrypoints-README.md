# Original XanimeCore matrix entry points

This root-only recovery appends four methods to
`src/Game/Animation/XanimeCore.cpp`. The parent supplied the missing method
declarations in `include/Game/Animation/XanimeCore.hpp`; no PC source, build
configuration, renderer owner, or test was changed by this task. The preceding
checkpoint was `d40387003`. Recovery used the current RMGK01 DOL, not historical
source bodies. The verified DOL SHA-1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`.

| Method | Retail address | Retail / compiled bytes | objdiff |
| --- | --- | --- | --- |
| `calcScaleBlendBasic` | `0x8001AB58` | 260 / 264 | 84.138460% |
| `calcScaleBlendMayaNoTransform` | `0x8001A4BC` | 460 / 460 | 99.826090% |
| `calc` | `0x8001AFA0` | 292 / 292 | 100% |
| `init` | `0x8001B0C4` | 36 / 36 | 100% |

Compilation used the configured GC 3.0a3 compiler and `configure.py`
`cflags_game` for RMGK01, through the existing Shift-JIS wrapper. The compiler
reported the existing warning about the nontrivial `J3DTransformInfo` union
member; there were no errors. No synthetic include overlay or replacement
class was used. The evidence records source-body hashes, DOL function hashes,
object/tool hashes, and all external references.

`calc`, `init`, and `calcScaleBlendMayaNoTransform` have identical complete
instruction streams after normalizing relocation constant names and relative
branch addresses. The Maya percentage difference is the compiler's local
label for the same `1.0f` constant. Registers and memory offsets also agree.

Basic's lower percentage is reported without calling it a high fuzzy match.
It has one extra instruction to form `mCurrentS`'s address, changed general
register allocation, and reordered independent loads/stores. Retail forms
the same address with `lfsu`; the source compiler uses `addi` plus `lfs`.
All three multiplications and three comparisons have exactly the same floating
register operands. Both versions write translation to matrix offsets
`0x0C/0x1C/0x2C`, multiply the corresponding three current-scale components,
test those products against `1.0f`, write the same scale flag, and call
`JMAMTXApplyScale`, `PSMTXConcat`, and `PSMTXCopy` with the same data and order.
All 13 normalized arithmetic/helper/control events, including branch-target
topology, agree. A local `Vec&` spelling was checked in an ignored build-only
probe and produced the same code; no unsafe `Vec` downcast was introduced to
chase register allocation. Functional correspondence is the completion basis
for this method, as allowed by `AGENT_DECOMP_GUIDE.md`.

Across all four functions, 87 ordered arithmetic/helper/control events agree.
This comparison supplements operand/source inspection; it is not a runtime
claim about the full animation subsystem.

## Literal dispatch and initialization contracts

`calc` first sets `j3dSys.mCurrentMtxCalc = this`. `_6 == 1` runs
`calcBlendSpecial` and returns; `_6 == 2` runs `calcScaleBlendSpecial` and
returns. Otherwise, exactly one track selects `calcSingle`; every other track
count selects `calcBlend`. `_6 == 3` then calls `fixT` on translation.

The matrix convention `_4` dispatch is:

| `_4` | Without `mTransformList` | With `mTransformList` |
| --- | --- | --- |
| 0 | Basic | Maya |
| 1 | Softimage | Softimage |
| 2 | Maya without transforms | Maya |

Mode 0 deliberately falls through to the Maya path when a transform list is
present. Unknown modes still execute the preceding pose blend but skip matrix
scaling/concatenation. No new default branch or error behavior was added.

`init` delegates all three recognized modes (0, 1, 2) to
`J3DMtxCalcJ3DSysInitMaya::init`. It does not choose the ordinary Softimage
initializer for mode 1. The called original SDK helper initializes parent scale
to one, records the supplied base scale as current scale, and applies that
scale to the supplied base matrix. Unknown modes do nothing.

## Scale contracts and required state

Basic uses the rotation already written in the current joint's animation
matrix by pose blending, writes the supplied translation, and multiplies
`J3DSys::mCurrentS` by local scale. The accumulated scale determines the scale
flag and whether the **local** scale is applied. If accumulated scale is exactly
one, retail skips local scaling even if that local scale is not one. The result
is concatenated into `mCurrentMtx` and copied back to the joint animation
matrix. This behavior must not be replaced with generic local-scale detection.

Maya without transforms uses local scale for its scale flag and scale
application. With `getScaleCompensate() == 1`, it separately compensates each
rotation row by `JMath::fastReciprocal` of that parent-scale component, only
when the component differs from `1.0f`. It preserves translation, performs the
same concatenation/copy, then replaces `mParentS` with the supplied local scale.
There is no zero-parent-scale check, epsilon, or fallback in retail. Negative,
zero, and nonfinite values retain the original helper and comparison behavior.

These methods require the original `J3DMtxCalc` current joint and matrix buffer,
the initialized `J3DSys` matrix/scale globals, and the existing source-backed
JMath/SDK helpers. `calc` additionally references the separately recovered
Maya/Softimage/special methods. No fake `J3DModel` is required for ordinary
constructor-default `_6 == 0` calculation. `_6 == 3` reaches `fixT`, which
requires `j3dSys.getModel()->getModelData()`; native coverage of that mode needs
a real typed model owner and is not established by this source task.

## Reproduction

Run `python3 pc-port/notes/xanime-core-matrix-calculation-20260903/entrypoints-verify-object.py`
from the repository. It compiles the real root translation unit, freshly splits
the verified DOL with dtk, and writes generated artifacts under
`build/xanime-core-matrix-calculation-20260903/entrypoints/`. The full aligned
comparison is `entrypoints-comparison.txt` there. The script checks direct call
order, constants, external globals, control/arithmetic topology, and the three
complete normalized instruction matches.

`entrypoints-verify-source.py` checks the four recorded root source-body hashes
independently of unrelated methods added to the shared translation unit.
`entrypoints-compiler-evidence.json` and `entrypoints-original-compiler.log`
retain the reviewed comparison and compiler output. Native builds and runtime
integration belong to the parent task.
