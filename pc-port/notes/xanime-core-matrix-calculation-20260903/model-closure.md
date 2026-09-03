# Real typed model closure for XanimeCore

This audit was made against checkpoint `d40387003` and the current original
XanimeCore restoration. `model-foundation.md` describes the bounded constructor
work subsequently implemented. Full J3DModel rendering and typed BMD loading
remain separate dependencies.

## What the original core actually requires

`XanimeCore::initT`, `enableJointTransform`, `reconfigJointTransform`, and
`freezeCopy` use the real ModelData joint count/table, each joint's bind transform,
and actual child/younger links. The existing real JointTree/Joint/MtxCalc/MtxBuffer
foundation supplies that tree and calculation state. The three restored scale
calculators use current joint/matrix-buffer state and the original J3DSys static
matrix/scale fields; they do not read a current model object.

`XanimeCore::fixT` is a distinct dependency. It gets `j3dSys.getModel()` and reads
that actual model's `mModelData` to restore bind translation for eligible joints.
The original `calc` selects this operation only in mode `_6 == 3`; `initMember`
sets `_6 = 0`. Tests of the original default core can use a complete actual
ModelData with intentionally joint-only contents without constructing a Model.
They must not claim that mode 3 or the complete XanimePlayer/model lifecycle is
available. A mode-3 caller needs a real selected J3DModel with the corresponding
joint data; a different global object or a layout cast is insufficient.

Original `XanimePlayer` is stricter: its constructor reads the actual model's
ModelData, flags, and joint count, and later binds the core into its actual
joints. That cannot be activated by passing a renderer summary as a Model.

## Constructible ModelData

The full original class has one virtual destructor and embeds, in order, real
JointTree, MaterialTable, ShapeTable, and VertexData objects. Its constructor and
destructor are a bounded import because the embedded types' required virtual
tables are small and their real lifecycle providers are available. The missing
VertexData constructor was recovered at `0x804237F4` and verified exactly.

The imported declarations retain all fields and virtual slots; PPC offset
comments are documentation, not native offsets. Arrays still belong to a
separate native owner because the original empty destructors do not free them.
An owner must populate the actual embedded joint tree and keep its table, joints,
hierarchy records, and calculator alive. It must not cast the existing
`OriginalJ3dJointTree::Storage`, or copy a pointer-owning tree and let the original
owner die. Publishing an authored BMD as a fully loaded typed ModelData also
requires the real decoded material, shape, and vertex data.

## Actual J3DSys object

Only the static traversal matrices/scales were previously provided natively.
The full J3DSys header already exists, but the real global instance requires the
original constructor and its four table builders:

| Original provider | RMGK01 address | Required state |
| --- | --- | --- |
| `J3DSys::J3DSys` in `J3DSys.cpp` | `0x804225D4` | Real global object, identity view, flags, draw mode, selected pointers, scale table |
| `makeTexCoordTable` in `J3DTevs.cpp` | `0x80430A64` | `j3dTexCoordTable[7623]` |
| `makeAlphaCmpTable` | `0x80430B5C` | `j3dAlphaCmpTable[768]` |
| `makeZModeTable` | `0x80430BC4` | `j3dZModeTable[96]` |
| `makeTevSwapTable` | `0x80430C2C` | `j3dTevSwapTableTable[1024]` |

These are existing original bodies and can be extracted without importing all
GX display-list methods. Retain one definition of `J3DSys::sTexCoordScaleTable`,
the existing traversal globals, and `j3dDefaultViewNo`; do not replace constructor
calls by zeroing a fabricated object. The real global's static zero initialization
and the original constructor together establish its initial state.

## J3DModel vtable and execution dependencies

The complete original Model header is already present natively. An actual
instance references all seven original virtual slots, even when an owner intends
to call only nonvirtual `calcAnmMtx`. Constructor-only extraction cannot provide
a real linkable instance while those slots lack providers.

| Virtual slot | Original body | Important direct dependencies |
| --- | --- | --- |
| `update` | `J3DModel.cpp` | Virtual `calc` and `entry` |
| `entry` | `J3DModel.cpp` | ModelData/J3DSys flags and texture; `J3DJoint::entryIn`, real material/draw-buffer/packet operations |
| `calc` | `J3DModel.cpp` | VertexBuffer frame state, deformation wrappers, `calcAnmMtx`, envelope calculation, optional callbacks |
| `calcMaterial` | `J3DModel.cpp` | Real MaterialAnm and Material calculations |
| `calcDiffTexMtx` | `J3DModel.cpp` | Material texture matrices and actual shape-packet texture-matrix objects |
| `viewCalc` | `J3DModel.cpp` | Draw/normal/envelope/billboard/bump matrices, shape packets, cache publication |
| Destructor | `Game/Player/J3DModelX.cpp` | Original empty body still invokes the embedded VertexBuffer destructor |

`J3DModelX.cpp` is currently excluded from native compilation, so its empty
J3DModel destructor is not an active provider. It must be imported as its actual
body, rather than assumed available.

The embedded VertexBuffer is a particularly bounded next decompilation task:

| Missing root method | RMGK01 address | Bytes |
| --- | --- | --- |
| `J3DVertexBuffer::setVertexData` | `0x80423874` | `0x48` |
| `J3DVertexBuffer::init` | `0x804238BC` | `0x40` |
| `J3DVertexBuffer::~J3DVertexBuffer` | `0x804238FC` | `0x40` |

The first is needed for original model-data attachment; the latter two are
required even by the default Model constructor/destructor. Original
`J3DModel::initialize` is already decompiled and establishes separate base scale
and base TR matrices.

The actual matrix entry point is original `J3DModel::calcAnmMtx`: select this model
in J3DSys, then call its real embedded JointTree with default scale/matrix when
`J3DMdlFlag_UseDefaultJ3D` is set, or with the model's base scale/TR otherwise.
Full `entryModelData` additionally creates matrix storage, shape and material
packets, binds vertex arrays, and prepares packet pointers. Calling that function
on an authored asset with empty placeholder material/shape tables would bypass
required loading and is not a valid incremental import.

Several wider links are source-backed but require actual implementations before
the corresponding Model vtable can be supplied:

- `J3DSkinDeform::deform(Model*)` and `J3DVtxColorCalc::calc(Model*)` are bounded
  existing wrappers around virtual operations. They do not themselves require
  constructing optional deformers, but their real wrapper bodies are needed.
- `J3DDeformData::deform(Model*)` leads through nonvirtual cluster/deformer methods
  in `J3DCluster.cpp`; it cannot be replaced by a null-only body merely because
  a particular model has no clusters.
- `J3DMtxBuffer::calcWeightEnvelopeMtx` contains original paired-single assembly
  in the root source. A portable implementation needs instruction-backed math
  verification. Buffer draw/normal/billboard and `J3DCalcViewBaseMtx` bodies are
  present but not yet all active natively.
- `J3DModel::calcNrmMtx` is declared and called by `viewCalc`, but has no root
  standalone body or named RMGK01 symbol found by this bounded audit. Its
  inlined retail sequence must be established before implementing it.
- Material, shape, packet, draw-buffer, and GX submission methods remain wider
  rendering work. Their absence must not be hidden by placeholder vtable bodies.

## Ownership and restoration requirements

The shared XanimeCore constructor aliases both the joint list and transform list;
track arrays remain per instance. Native ownership must preserve those lifetimes
and avoid double deletion, while the original destructor remains unchanged.

Existing native tree calculation restores current matrix/scale, joint,
calculator, and matrix-buffer globals for nested/exception-safe calculations.
Once an actual Model is selected, that owner boundary must also save and restore
`j3dSys.mModel`; the previous tree-only scope did not need that field. This is
especially relevant to nested callbacks and mode-3 bind-translation lookup.

The safe sequence is therefore: finish real data construction and original
default core matrix tests; supply the real J3DSys constructor/table state; close
the Model's actual virtual and embedded-object providers; then attach genuinely
decoded typed resources and activate the original Model/Player lifecycle. Each
boundary is useful independently and does not require pretending a later one is
complete.
