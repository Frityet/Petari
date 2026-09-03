# Original joint traversal and scale-mode tests

`pc-port/tests/OriginalJ3DJointTraversalTests.cpp` supplies nine focused groups
for `smg-pc-original-j3d-joint-traversal-tests`. The parent owns target wiring,
native builds, and execution. The test author has checked the source and
`git diff --check`; execution results belong to the parent checkpoint evidence.

Fixtures use real `J3DJoint`, `J3DJointTree`, `J3DMtxBuffer`, and the original
`J3DMtxCalcNoAnm` Basic/Softimage/Maya template instances. Test-owned arrays are
assigned through the original public matrix-buffer fields. This follows the
actual calculation contract without constructing incomplete model/resource
holders. Recording calculator subclasses only record virtual calls and then
delegate to the original implementation. Callbacks use the original
`J3DJoint::setCallBack` and `mCallBackUserData` surface; no production test hook
is introduced.

## Source and retail traversal evidence

Root `src/JSystem/J3DGraphAnimator/J3DJointTree.cpp::calc` initializes the basic
calculator and chooses the matrix buffer before checking for a null root. For
a nonnull root it selects the basic calculator and enters `recursiveCalc`.
Per-joint overrides run `calc`, but do not run their initializer.

Independent disassembly of RMGK01 `recursiveCalc` at `0x80438B1C`, size `0x148`,
established the following order before the imported implementation was read:

1. Save `J3DSys::mCurrentMtx`, `mCurrentS`, and `mParentS`.
2. Select a local override if present, otherwise inherit the current calculator;
   set the calculator joint pointer and call `calc`.
3. Cache the joint callback and invoke phase zero.
4. Recurse through the child.
5. Restore the three saved globals, then restore the previous calculator only
   if that saved pointer is nonnull.
6. Invoke phase one through the cached callback.
7. Recurse through the younger sibling.

The retail function ignores callback return values. It does not restore
`J3DMtxCalc::mJoint`. Phase one sees the incoming parent matrix/scales and can
change the state inherited by a younger sibling. Replacing a callback during
phase zero does not change the already-cached phase-one call. If an override
was entered with a null previous calculator, it remains the current calculator
after traversal because restoration is guarded by a nonnull check.

Disassembly is available at ignored
`build/original-j3d-joint-traversal-20260903/recursive_calc.asm`; it was produced
with `build/compat-math-oracle/disassemble_dol.py` from the supplied RMGK01 DOL,
SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`. The source restoration and
original-compiler correspondence are owned and documented by the importing
agent. This note records the independent test derivation.

## Nine regression groups

1. Empty-tree initialization proves Basic/Maya bake model scale into the
   initial matrix while Softimage keeps it separate. Basic/Maya reset parent
   scale to one; Softimage leaves it unchanged. The null-root path still
   selects the buffer and initializes once, but does not calculate a joint or
   replace the current joint-calculator pointer.
2. A root, overridden child, grandchild, ordinary child sibling, and second root
   prove exact call ordering, calculator inheritance/restoration, matrix and
   accumulated-scale restoration, and phase-one static-joint behavior.
3. Actual callbacks modify the current matrix. Phase-zero changes reach
   children, phase-one changes reach younger siblings, and stored joint output
   is not implicitly rewritten by those global changes. A phase-zero callback
   replacement still receives its old cached phase-one call. Returning zero
   does not stop traversal.
4. Literal nonuniform scale and quarter-turn matrices distinguish all three
   modes, including Maya segment scale compensation and independent sibling
   restoration. Current traversal matrices, stored matrices, accumulated scale,
   direct-parent scale, and scale flags are checked separately.
5. Scale cancellation distinguishes the original Basic branch from Softimage
   and Maya: when accumulated scale reaches exactly one, Basic skips applying
   the local scale even though its incoming matrix remains scaled. The final
   fixture also tests the original null-previous-calculator restoration guard.
6. The actual `OriginalJ3dJointTree` owner retains original joint links,
   hierarchy commands, joint fields, and matrix storage after the source
   summaries are overwritten and destroyed. Material/shape commands between a
   joint and a begin-child command do not clear that current joint. Parsed
   summary fields have already applied the JNT1 remap; a deliberately different
   remap table and unusable old parent-index summary prove the owner consumes
   final decoded fields and original INF1 commands. All three original low-bit
   flag modes select their actual original calculator and produce independently
   expected matrices.
7. An actual original Key object with caller-owned native tables is borrowed
   only during owner calculation. Negative, fractional, and final raw frames
   preserve endpoint/interpolation behavior without renderer wrapping, while
   the object's own frame remains unchanged. The owner restores every outer
   traversal pointer, matrix, and scale on success and after a callback throws
   while changing all those globals. Its original basic calculator is restored,
   and the owner returns to bind pose after the borrowed animation dies.
8. A callback can calculate a different joint owner and resume with its outer
   globals unchanged. Same-owner reentry is rejected before mutating its shared
   animation adapter or buffer; exception cleanup permits the next ordinary
   calculation to succeed.
9. Owner calculation accepts an actual translated/rotated base matrix and a
   separate nonuniform base scale. All three original modes receive those
   values through their initializer for both JNT1 bind transforms and a real
   original Key animation. Literal matrices distinguish ordinary nonuniform
   scale from Basic's exact accumulated-scale cancellation, and a rotated child
   proves the resulting mode-specific parent state continues through traversal.

The owner review identified that a recursive mutex alone did not protect
same-owner animation reentry: an inner calculation overwrote the outer borrowed
animation/frame and cleared its pointer before the outer traversal resumed.
The parent added the explicit in-progress rejection and exception-safe reset;
group eight exercises that boundary. Different-owner nesting remains supported.

## Independently derived scale expectations

The rotation fixture starts at identity. Its root has scale `(2,4,8)`, a
90-degree Z rotation, and translation `(10,20,30)`. Its child has scale
`(4,2,8)`, another 90-degree Z rotation, and translation `(1,2,3)`.

All modes emit root basis `Rz * diag(2,4,8)`. Basic and uncompensated Maya emit
child diagonal `(-16,-4,64)` at position `(2,22,54)`. Softimage retains a pure
rotation in its current matrix and applies cumulative scale only to output,
so its child diagonal is `(-8,-8,64)` at the same position. A unit-scale
grandchild translated by `(1,1,1)` reaches `(-14,18,118)` in Basic versus
`(-6,14,118)` in Softimage.

Maya divides the rows of the child's local basis by the direct parent's scale,
but leaves local translation untouched. The retail helper is bare PowerPC
`fres`, so mathematical division would be an inaccurate expected value. For
the powers of two in this fixture, let
`r = 0.9998779296875f` (bits `0x3F7FF800`). The importing agent verified those
estimate values independently against the retail instruction and local Dolphin
oracle. Compensated child output has diagonal `(-4r,-2r,8r)` at `(2,22,54)`.
The compensated unit grandchild has diagonal `(-r²,-r²,r²)` and position
`(2-4r,22-2r,54+8r)`, where the rounded float `r²` is `0.999755859375f`.
These constants are independent test expectations; the test does not call the
production reciprocal helper to calculate its oracle.

Basic/Softimage scale flags test cumulative scale, while Maya tests the local
transform's scale. Maya updates only the direct-parent scale during calculation;
its current-scale vector remains the model scale selected at initialization.

The owner base-transform fixture supplies a 90-degree Z rotation, translation
`(100,200,300)`, and separate scale `(2,4,8)`. Its root rotates 90 degrees about
X and translates by `(1,2,3)`. With local scale `(3,5,7)`, the root basis is
`[0,0,28; 6,0,0; 0,40,0]` in Basic/Maya versus
`[0,0,56; 6,0,0; 0,20,0]` in Softimage. All root positions are `(92,202,324)`.
These values are derived by multiplying the stated axis rotations and diagonal
scales according to the original mode contract, not by calling an owner helper.

Changing root local scale to `(1/2,1/4,1/8)` cancels the accumulated scale.
Basic skips local scale and retains basis `[0,0,4; 2,0,0; 0,8,0]`; Softimage
emits `[0,0,1; 1,0,0; 0,1,0]`; Maya emits
`[0,0,1/2; 1,0,0; 0,2,0]`. A child with 90-degree Y rotation, local scale
`(3,5,7)`, and translation `(4,5,6)` then reaches position `(116,210,364)`,
`(98,206,329)`, or `(95,206,334)` respectively. The test asserts those full
matrices and root scale flags for both bind and animation sampling paths.

The tests restore shared J3D globals explicitly on exit. The original recursion
is not a general scope guard: it leaves the selected buffer, calculator joint,
and some calculator-selection state live. A renderer owner must preserve its
outer execution context separately. These tests establish the original joint
traversal boundary, not a complete J3D model/draw or Xanime lifecycle.
