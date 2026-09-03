# Original Xanime resource and ownership foundation

Follow-up: `../original-j3d-transform-animation-20260903/` supplies owned typed
BCK/BCA resources and original samplers, now used for live BCK rendering.
`../xanime-core-lifecycle-restoration-20260903/` recovers the bounded root core
lifecycle routines below. The complete original player, weighted pose blending
and joint/model traversal remain unfinished; the original checkpoint record
below describes the state before these follow-ups.

This tranche imports the complete original `XanimeResourceTable` implementation and its original hash-table dependency. It does not activate Mario's original animator/player/core lifecycle yet. The existing Run/Wait native BCK path is still the active Mario animation path, so this work is preparation for original jump/landing animation state, not evidence that jump animation now works.

## Changes and verification

- `src/Game/Animation/XanimeResource.cpp` was copied to `pc-port/src/Game/Animation/XanimeResource.cpp`. Its existing PC header already matches the root header exactly.
- Two mechanical changes are confined to the imported Animation unit: scope the local variable under `getGroupInfo`'s switch case, and access animation metadata through the original `J3DAnmTransform::getAttribute()/getFrameMax()` members instead of offsets into a Wii virtual object.
- `compat/HashSortTableCompat.cpp` extracts all ten original `HashSortTable` methods from root `src/Game/Util/HashUtil.cpp`, plus the exact unsigned `MR::sortSmall` body from root `src/Game/Util/MathUtil.cpp`. Whole-unit imports would conflict with the existing hash/string/math providers.
- Seven `XanimeResourceTable` query bodies and the hash-table `search` body were removed from `XanimeQueryCompat.cpp`; the two Player query methods remain until the full Player owner is available. The parent task separately imported `ResourceInfo.cpp`, removed its four query extracts, and supplied the original `J3DAnmBase/J3DAnmTransform` declarations and transform constructor.

Run `python3 pc-port/notes/original-xanime-lifecycle-20260903/verify-resource-import.py` from the repository root. The script compiles both the untouched root Resource unit and its native import with the configured original **GC/3.0a3** compiler through `build/tools/wibo`. All **18 methods, 885 PPC instructions and 81 relocation records are identical**. It also checks byte-for-byte correspondence for the extracted hash/sort helper bodies. Results are in `resource-import-evidence.json`; compiler commands and logs are in `build/compat-xanime-resource/`.

The ABI correction is independently supported by the verified **RMGK01 rev0** DOL, SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`: `initGroupInfo` at `0x8001C988` reads the animation attribute with `lbz r3,4(r4)` at `0x8001CCE8` and signed end frame with `lha r0,6(r4)` at `0x8001CCFC`. These are exactly `J3DAnmBase::mAttribute/mFrameMax` in the original SDK header. A native virtual pointer has a different width; typed members preserve the fields. `build/compat-xanime-resource/init_group_info.asm` retains the full disassembly.

The compiler comparison proves that the native edits preserve the current root algorithm on Wii. It is **not** a new whole-unit retail matching claim for the existing root decompilation.

`tests/HashSortTableTests.cpp` contains two groups in `smg-pc-hash-sort-table-tests`: unsigned bucket boundaries, preservation of payloads through sorting, absent/null-output lookup, duplicate suppression, composite-name lookup, and rename/add followed by sorting again. The fixture constructs the actual original table and owns its allocated arrays. No fake ResourceHolder is constructed. The parent reports that the showcase, live-walk slice and both table test targets built successfully, with all three ResourceTable groups and both HashSortTable groups passing. This agent ran only the original-compiler/source verification; the parent owns native build/runtime logs and final live-walk validation.

## Required resource contract

The original resource algorithm already supplies authored group names, weights, offsets, loop/start/end frames and flags. There is no need for a native table mapping jump names to BCK files.

The current `compat/ResourceHolderCompat.hpp` and `Game/System/ResourceHolder.hpp` define incompatible global `ResourceHolder` classes. The former owns a native archive pointer/path; the latter has the actual `ResTable*` fields used by Xanime. A native archive holder cannot be passed to the original Resource constructor merely because its C++ type name matches. The parent identified the same ownership/layout conflict and owns its migration. Until then, this imported unit is usable only when supplied a properly constructed original-shaped resource holder with actual loaded entries; no such actor holder integration is claimed here.

Root `ResourceHolder::createAndRegisterObject` establishes the contract:

1. `.bck` and `.bca` motion entries contain loaded **`J3DAnmTransform` objects**, not raw file bytes. `J3DAnmLoaderDataBase::load` determines the actual Key/Full class.
2. `ResTable::add(name, loadedObject, true)` strips the extension and stores the original case-insensitive hash. `ResFileInfo::mResource` points at the loaded object; `_8` retains raw resource bytes, `_4` their size and `_C` their archive file ID.
3. Xanime retains these object pointers in each group's `_20` array. The holder, loaded transform objects, native-endian key/value arrays and raw archive data must all remain alive through every dependent resource table/player/calculator.
4. Non-animation tables must also obey their original typed contract when exposed. Populating `mModelResTable` with BDL bytes would repeat the same problem: original callers expect real `J3DModelData`, joints and materials.

The original root loader/sampler sources are `src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp` and `src/JSystem/J3DGraphAnimator/J3DAnimation.cpp`; their original headers are under `libs/JSystem/include/`. `J3DAnmTransformKey::getTransform` calls `calcTransform(mFrame,...)`, using actual keyed interpolation and the stored frame directly. The current renderer's `j3d_evaluate_bck_joint_transform` first applies separate frame wrapping. Reusing that entry point as the original virtual `getTransform` would change endpoint/loop behavior, because Xanime already controls each transform's frame.

The imported Resource unit's external method dependencies are `ResTable::findFileInfo/isExistRes/getRes`, `HashSortTable` construction/add/sort/search, and `MR::getHashCode/strcasecmp/extractString`, plus allocation and standard `strcmp`. Original `findFileInfo/isExistRes/getRes` reach the original resource-name lookup through `getResIndex`. There is no J3D joint dependency in Resource construction itself.

## Next original Player/Core boundary

`MarioAnimator::init` passes the authored `marioAnimeTable`, `marioAnimeAuxTable`, `marioAnimeOfsTable`, single/double/triple/quad BCK tables and optional Luigi swap table from `MarioAnimatorData.hpp` to the original Resource constructor. That constructor **mutates** the group/single tables with motion pointers, metadata and hashes. The owner must retain the tables and consider independent instances or model/resource replacement explicitly; it must not leave groups pointing at a retired holder. The original resource class has no destructor. Its hash arrays and optional simple-group allocations need owner cleanup at native teardown; the Game algorithm need not change to supply that ownership.

Root `XanimePlayer.cpp` has the animation transition, frame-control and query bodies. Player construction immediately creates an actual `XanimeCore`, reads `J3DModelData::mFlags/mJointTree.mJointNum`, and calls `initT(modelData)`. Activation calls `setBck`, `updateFrame` and installs the calculator into the actual joint's `mMtxCalc`. The original renderer/model traversal must then execute this callback. A player containing name and frame fields alone would not implement that lifecycle.

`XanimeCore.cpp` currently contains only constructors, basic initialization/weight/freeze setters and its empty original destructor. These missing retail routines are required before complete original Player/Core construction and drawing can be claimed:

| Routine | RMGK01 address | Size |
| --- | --- | --- |
| `XjointTransform` constructor | `0x80019060` | `0x90` |
| `enableJointTransform` | `0x800190FC` | `0x124` |
| `reconfigJointTransform` | `0x80019220` | `0x7C` |
| `setBck` | `0x80019460` | `0x38` |
| `calcBlend` / `calcSingle` | `0x800194AC` / `0x800198C0` | `0x414` / `0x1CC` |
| `calcBlendSpecial` | `0x80019A8C` | `0x41C` |
| `updateFrame` | `0x80019EA8` | `0xCC` |
| `calcScaleBlendMaya` | `0x80019F74` | `0x548` |
| `calcScaleBlendMayaNoTransform` | `0x8001A4BC` | `0x1CC` |
| `calcScaleBlendSI` | `0x8001A688` | `0x4D0` |
| `calcScaleBlendBasic` | `0x8001AB58` | `0x104` |
| `calcScaleBlendSpecial` | `0x8001AC5C` | `0x98` |
| `freezeCopy` | `0x8001ACF4` | `0xEC` |
| `initT` / `fixT` | `0x8001ADE0` / `0x8001AEC8` | `0xE8` / `0x48` |
| virtual `calc` / `init` | `0x8001AFA0` / `0x8001B0C4` | `0x124` / `0x24` |

`freezeCopy` has a historical body in commit `3c0aaf0f0` that can be restored and checked against retail before new decompilation. The large blend/calculation routines are absent there and in the examined premerge `96e5ef0` snapshot. `XtransformInfo::operator=` at `0x80018E10` (`0x68`) also needs closure. Two methods already exist in different root units: `XanimeCore::getJointTransform` in `Mario.cpp`, and `XanimePlayer::tellAnimationFrame` in `MarioAnimator.cpp`; avoid duplicate definitions during activation.

The lower and upper core constructors deliberately share the joint-info and optional transform arrays while allocating separate track lists. Native cleanup must free each shared allocation once. Original `MarioAnimator::calc` installs separate lower/upper joint calculators and executes two model-calculation passes with `_6=1` then `_6=2`. Preserve this callback/traversal sequencing instead of rendering one selected BCK. Its root use of `joint + 0x54` is another definite pointer-width correction to make when that Player routine is actually activated, using the original typed `mMtxCalc` member.

The next bounded work is a genuine typed BCK/BCA resource owner with original sampling, followed by original J3D joint/model traversal and root-first core restoration. The player constructor and its authored tables can then be activated as one coherent owner, with tests for authored transitions, weighted tracks, shared upper/lower state, stopped/end frames and model replacement.
