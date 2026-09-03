# Original XanimeCore lifecycle recovery — 2026-09-03

Recovered nine bounded lifecycle methods in root `XanimeCore.cpp/.hpp`, using
the verified **RMGK01** executable and configured original compiler. No PC
imports, native build configuration, tests, or activation were changed. The
large blending and joint-calculation methods remain undecompiled; the class
must not be presented as a complete working native animator yet.

## Live original-compiler evidence

Target: `build/compat-math-oracle/main.dol`, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`. `dtk v1.8.3` splits this executable
using the actual `config/RMGK01/symbols.txt` and `splits.txt`, with `--no-update`
and output under ignored `build/xanime-core-lifecycle-restoration-20260903/`.
The root source compiles with **GC/3.0a3**, the repository's `cflags_game`,
`VERSION=0`, `wibo`, and `sjiswrap v1.2.2`. `objdiff-cli v3.6.1` then compares
the root object with that retail object.

| Recovered method | Retail address | Retail/compiled bytes | Live match |
| --- | --- | --- | --- |
| `XtransformInfo::operator=` (compiler-generated) | `0x80018E10` | 104 / 104 | 100.000000% |
| `XjointTransform` constructor | `0x80019060` | 144 / 144 | 99.722220% |
| `enableJointTransform` | `0x800190FC` | 292 / 292 | 99.041100% |
| `reconfigJointTransform` | `0x80019220` | 124 / 124 | 100.000000% |
| `setBck` | `0x80019460` | 56 / 56 | 99.642860% |
| `updateFrame` | `0x80019EA8` | 204 / 204 | 98.529410% |
| `freezeCopy` | `0x8001ACF4` | 236 / 236 | 90.847460% |
| `initT` | `0x8001ADE0` | 232 / 232 | 99.827580% |
| `fixT` | `0x8001AEC8` | 72 / 76 | 93.888885% |

All listed methods meet the decomp guide's functional/high-fuzzy threshold.
These are current relocation-aware comparisons, not a whole-class binary
matching claim. The verifier also checks actual PPC sizes/offsets, referenced
constant bits, constructor/data pointers, and helper call order. The only
allowed call-name adaptation is the typed `Vec` copy explained below.

The original compiler emits one warning for the union containing
`J3DTransformInfo`, whose existing SDK assignment operator is nontrivial.
The union constructor/storage is usable by the original compiler and the
verified layout is unchanged. Its original scalar aliases retain the exact
zero initialization of the two padding bytes at `0x56`; `original-compiler.log`
records the warning. No new compiler errors remain.

## Data declarations recovered from instructions

The root header previously described several compound fields only as unrelated
scalars or an integer. The new typed views are supported directly by retail:

- `XtransformInfo` is `0x28` bytes. It contains scale at `0`, translation at
  `0xC`, and a `Quaternion` at `0x18`. `initT` passes `+0x18` and `+0x40` to
  `Quaternion::operator=` for the two `XjointInfo` slots. A union preserves the
  existing `_18/_1C/_20/_24` scalar names.
- `XtransformInfo::operator=` was incorrectly user-declared but undefined.
  Removing that declaration lets the original compiler generate its assignment:
  two actual vector assignments followed by a 16-byte quaternion-storage copy.
  That emitted method has a 100% retail comparison. There is no hand-written
  float-copy replacement or missing link symbol.
- `XjointTransform::_0` is a real `J3DJoint*`, not a 32-bit integer. Retail stores
  the model's joint pointer there, then reads that joint's child/sibling pointers
  while determining `_4`. `_4` is an unsigned 16-bit index with `0xFFFF` sentinel.
- `XjointTransform + 0x44` is a full `J3DTransformInfo`. `enableJointTransform`
  and `reconfigJointTransform` call its actual SDK assignment operator with the
  source joint's `mTransformInfo` at joint offset `0x18`. The existing field
  aliases remain in a union; no extra storage was introduced.
- `fixT(TVec3f*)` was absent from the header and is now declared.

The original-compiler layout probe checks:

| Type/field | Verified PPC value |
| --- | --- |
| `sizeof(XtransformInfo)` | `0x28` |
| `sizeof(XjointInfo)` | `0x64` |
| `sizeof(XjointTransform)` | `0x70` |
| `sizeof(XanimeTrack)` | `0x10` |
| `sizeof(XanimeCore)` | `0x2C` |
| `XjointTransform::mTransformInfo`, `_56`, `_64`, `_6C` | `0x44`, `0x56`, `0x64`, `0x6C` |
| `XanimeCore::mJointList`, `mTransformList`, `mTrackList` | `0x10`, `0x14`, `0x18` |

The root source uses real SDK `getJointNodePointer`, `getTransformInfo`,
`getChild`, `getYounger`, and `getJntNo` accessors. It does not construct fake
model/joint objects or cast an integer to a native joint pointer.

## Exact lifecycle contracts

`XjointTransform` starts with no joint or override matrices, parent index
`0xFFFF`, unit `mScale` and `_14`, and zero values for the remaining transform
fields and padding. It does not allocate any override matrices.

`enableJointTransform` allocates and constructs one transform per `mJointCount`.
For each model joint in order it stores the actual joint pointer and authored
SRT, then searches the allocated records. A record whose child is the current
joint gives its index as the parent; a record whose younger sibling is the
current joint gives its own known parent. Unpopulated records have null joint
pointers and are skipped. This is the original one-pass algorithm, including
its dependence on the model's joint ordering. Root and unresolved parent
sentinels remain as in retail. No alternate tree-building algorithm was added.

`reconfigJointTransform` only replaces joint pointers and authored SRT values.
It does **not** allocate a replacement array or recompute the parent indices.
The original caller therefore requires compatible joint count/topology and a
previously allocated transform list when replacing its model.

`setBck` stores the actual `J3DAnmTransform*`, writes track `_8 = 0.0f`, writes
track `_C = 1`, then writes `_C = 0` through the core's track-list access. Both
writes are visible in retail at `0x80019484` and `0x80019490` and are retained.
The local track reference preserves their original reload/write grouping.

`updateFrame` skips null tracks. For every nonnull transform it writes the
animation object's frame to `mFrameMax * track._8` when `_C` is set, otherwise
to `mFrameMax * core.mFrameRatio`. `mFrameMax` is signed 16-bit, as read by
retail's `lha` at `0x80019EE4/0x80019F0C`. It does not advance, loop, clamp, or
sample that frame. Afterwards pending freeze `_28` becomes the one-frame `_29`
flag and is cleared; otherwise `_29` clears. The actual Player/frame-control
owner supplies the ratio before the real sampler consumes the transform frame.

`initT` uses each model joint's authored scale, translation, and signed Euler
rotation. It calls the existing root `JMAEulerToQuat`, then initializes both
`XjointInfo` transform slots with the same values. It does not sample a BCK.

`fixT` gets the current joint from the actual `J3DMtxCalc::mJoint` and model data
from `j3dSys.mModel`. It leaves the vector alone for joint zero and core `_C`;
otherwise it restores that joint's authored translation. It requires the real
model traversal's current joint/model context; a synthetic identity context
would not satisfy this contract.

### Historical freezeCopy error corrected

Historical commit `3c0aaf0f09bb1470e812f7bda062472342439a73` has a body, but its
central assignment is wrong. It assigns `this->_28 = other->_28`. The current
retail executable instead does:

```cpp
pOther->mJointList[jointIndex]._0 = mJointList[jointIndex]._28;
pOther->mJointList[jointIndex]._5C = 0.0f;
pOther->mJointList[jointIndex]._60 = interp ? 1.0f / interp : 1.0f;
```

At `0x8001AD64..0x8001AD7C`, retail loads `this->mJointList` from `r26+0x10`
and `other->mJointList` from `r28+0x10`, makes `r4 = this + index*0x64 + 0x28`
and `r3 = other + index*0x64`, then calls the assignment operator. This proves
both the copy direction and destination slot. Subsequent stores at
`0x8001AD94` and `0x8001ADC8` update the other core's interpolation values.

The method recursively processes the selected joint's children and younger
siblings before copying the selected joint. Historical `mYoung/mJointNo`
names were adapted to the actual current SDK `mYounger/mJntNo` accessors.
Source uses the same first-child call and following sibling loop; its remaining
90.85% difference is equivalent loop branch placement. No historical match
percentage is claimed as current evidence.

## Shared ownership and dependencies for later activation

The existing original core constructors allocate one `XjointInfo[]` for a
primary core, share that array in the secondary core, and allocate separate
track arrays for both. The secondary constructor also copies the optional
transform-list pointer; `shareJointTransform` can later replace it with the
primary list. `MarioAnimator.cpp:76` enables the primary transforms and line82
shares them with the upper core. Both actual Player constructors call `initT`
after constructing their core (`XanimePlayer.cpp:48/85`). The original Mario
upper-to-lower handoff calls `freezeCopy` at `MarioAnimator.cpp:140`.

The native owner must keep the original model data, joints, transform resources,
shared joint/transform arrays, and both independent track arrays alive. Existing
`XanimeCore::~XanimeCore` has an empty source body; cleanup ownership must free
shared allocations once. This recovery adds no independent owner or cleanup
policy inside Game code.

Dependencies used by these methods are actual J3D model/joint accessors,
`J3DTransformInfo` assignment, vector/Quaternion copies, allocation, and the
existing `JMAEulerToQuat` implementation in `src/JSystem/JMath/JMath.cpp:4`.
Its declaration is missing from the current shared math header, so this TU has
a narrow forward declaration. No math algorithm was changed. The native side
will also need real `J3DMtxCalc::mJoint` and `j3dSys.mModel` from joint traversal.

An existing mismatch remains outside the nine recovered methods:
`initMember` currently uses a trivial `new XanimeTrack[count]`, which omits the
retail allocation cookie and `__construct_new_array` call. The current method
compares 57.60%, while its field initialization, per-track `init`, and virtual
track-zero weight assignment correspond to retail. This pre-existing allocation
ABI discrepancy was not obscured by adding a fake runtime helper or changing a
track's lifetime speculatively. The actual original host compiler must own its
native new[]/delete[] representation. The large `calcBlend/calcSingle`, special
blends, scale blends, virtual `calc`, and virtual `init` remain absent from root;
they still prevent complete core/player/model-calculator activation. Existing
`getJointTransform` is provided by root `Mario.cpp`, so future imports must
avoid duplicating it.

## Remaining compiler differences

Constructor and `setBck` differences are local constant-label identities for
the same 0.0f/1.0f bits. `enableJointTransform` and `updateFrame` differ in
register allocation; the latter has identical signed conversion, multiplication,
frame stores and flag transitions. `freezeCopy` retains equivalent sibling-loop
branch placement, and `fixT` retains a four-byte early-return branch difference.

For `initT` and `fixT`, the SDK declares SRT fields as `Vec`, while Xanime uses
`TVec3f`. The recovery uses `TVec3::set(const Vec&)` to copy x/y/z, instead of
downcasting a base `Vec` that is not an actual `TVec3f` object. The emitted
helper has three scalar loads and three stores at exactly offsets0/4/8 with
no arithmetic; the verifier checks those instructions. Retail calls its vector
assignment helper. This is a documented typed-copy difference, not a new
sampling or blending algorithm, and no bit-identical claim is made for those
helper bodies.

## Reproduce and artifact map

```sh
python3 pc-port/notes/xanime-core-lifecycle-restoration-20260903/verify-source.py
python3 pc-port/notes/xanime-core-lifecycle-restoration-20260903/verify-object.py
```

The object verifier requires the ignored compiler and tool versions described
above. It compiles the actual root TU with production includes, runs a small
original-compiler layout probe, regenerates the retail split with `--no-update`,
then writes the full objdiff and aligned nine-method comparison under
`build/xanime-core-lifecycle-restoration-20260903/`. It reuses the ELF/DOL readers
from the recorded Mario update verifier; it does not run Mario's verification
task or a native build.

`source-correspondence.json` records the baseline commit, every current method
body (or implicit assignment's class declaration), historical freezeCopy hash,
retail ranges and function hashes, and declaration/typed-copy adaptations.
`compiler-evidence.json` records the live results, verified layouts, tool hashes,
and complete reference/call inventories. `original-compiler.log` records the
known union warning. These are source/compile checks, not runtime gameplay tests.
