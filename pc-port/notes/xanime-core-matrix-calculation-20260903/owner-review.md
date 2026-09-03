# Independent owner and quaternion review

Reviewed the current changes to `OriginalJ3dJointTree.cpp` and
`JMathQuaternionCompat.cpp` against the original XanimeCore/Key/JMath bodies.
This was a bounded source review; parent owns native build/runtime evidence.
No new correctness defect was identified in this integration.

The native tree owns one actual ordinary XanimeCore and explicitly retains the
joint/track arrays allocated by its original constructor. The default core
does not allocate a transform array or share its arrays with another core.
Those typed owners retire their arrays before the original empty core destructor
runs; neither the tree nor the Key destructor independently frees them.

Each calculation copies the actual SDK Key object into a stack Key with the same
typed payload pointers and a private frame. Original Key construction, generated
copy assignment, frame assignment, and `setBck` perform no allocation or throwing
operations. The cleanup scope is therefore established before any callback can
throw. It resets the basic calculator, clears the core's borrowed Key pointer,
and releases the reentry flag before the stack Key is destroyed. The source
animation's frame remains untouched.

Same-owner reentry is rejected under the recursive traversal lock before any
animation mutation. A different owner may nest: matrix/scale, current joint,
matrix buffer, J3DJoint calculator, and the newly written
`j3dSys.mCurrentMtxCalc` are restored on return or exception. The original default
mode `_6 == 0` does not select, access, or modify `j3dSys.mModel`; mode 3 remains
outside this owner and needs the real Model lifecycle described in
`model-closure.md`.

The JMath quaternion imports preserve the root bodies. Euler half-angle
arguments retain signed integer division and original lookup helpers. Implicit
contraction is disabled to match the original JSystem flags. The native
quaternion dot product rounds the two XY products, fuses the two ZW products
into those lanes, then adds the lanes, matching the paired-single sequence.
The sign predicate is evaluated before stores. Each subsequent quaternion output
reads only its corresponding source components, so the ordinary complete-object
aliases `dst == p`, `dst == q`, and `p == q == dst` are preserved.

The change is an actual core-driven joint calculation path. It does not construct
a fake ModelData/Model, supply missing rendering virtuals, or activate the full
XanimePlayer resource and transition lifecycle.
