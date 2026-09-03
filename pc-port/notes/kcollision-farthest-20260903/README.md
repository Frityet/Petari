# KCollisionServer farthest vertex recovery

Restored `KCollisionServer::calcFarthestVertexDistance` in the root decompilation
and corrected its declaration from `void` to `bool`. The existing root source
contained only an empty, commented skeleton. No PC runtime or build files changed.

## Retail evidence

RMGK01 rev0 symbol `calcFarthestVertexDistance__16KCollisionServerFv` is at
`0x80183208`, size `0x188`. The locally extracted `main.dol` SHA-1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`. The starting maximum is positive zero:
the `lfs` at `0x8018322c` loads `0x00000000` from `0x806bc04c`, relative to
`r2 = 0x806bfc20`.

The recovered order is significant:

1. Iterate every prism index from zero to the unsigned triangle count. The stored
   prism array has its original leading sentinel, so index `i` uses prism `i + 1`.
2. Read the prism's `JMapInfoIter`. An invalid iterator clears the eventual return
   value, without stopping the loop or preventing geometry processing. A valid
   iterator calls `MR::getCollisionDirector()->mCode->getCameraID(iter)`, then
   `MR::registerCameraCode`. This registers referenced prism attributes; unused PA
   rows are not visited. Missing `camera_id` retains `CollisionCode`'s existing
   unsigned `-1` result and still reaches the registration provider.
3. Check `isNearParallelNormal`. If true, assign `-MR::abs(prism->mHeight)` and
   exclude that prism from the radius. The retail `fabs`/`fneg` sequence at
   `0x80183300`/`0x80183304` preserves this mutation even for already-negative
   heights, and makes zero negative. Camera registration has already occurred.
4. Otherwise visit all three vertices through the existing `getPos`. Compare
   their squared magnitudes using a strict less-than test, retaining the maximum.
5. Store `MR::sqrt(maxDistance)` in `mMaxVertexDistance` and return whether every
   attribute iterator was valid. An empty mesh produces radius zero and `true`.

The return is explicit: `r29` starts at 1 (`0x80183234`), invalid attributes clear
it (`0x801832cc`), and `mr r3,r29` returns it (`0x8018336c`). Existing root callers
ignore the result, so correcting the declaration does not change their flow.

The registration boundary is the existing root `CollisionParts::init`: it calls
`initCameraCodeCollection(sensor->mHost->mName, mZone->mZoneID)`, this function,
then `termCameraCodeCollection`. Registration and naming must preserve that scope.

## Original compiler validation

Downloaded the repository-configured compiler archive (`20251118`) into ignored
`build/compilers` and its macOS wibo `1.0.3` into ignored `build/tools/wibo`.
Compiled the entire root `KCollision.cpp` with GC/3.0a3 and the original Game
flags from `configure.py`; compilation succeeded with no diagnostics. No xmake
or native application build was run by this task.

Reproduction, after those standard repository tools are present:

```sh
build/tools/wibo build/compilers/GC/3.0a3/mwcceppc.exe \
  -nodefaults -proc gekko -align powerpc -enum int -fp hardware \
  -Cpp_exceptions off -O4,s -inline auto -pragma 'cats off' \
  -pragma 'warn_notinlined off' -maxerrors 1 -nosyspath -RTTI off \
  -str reuse -enc SJIS -sdata 4 -sdata2 4 -ipa file -sym on \
  -i include -i libs/JSystem/include -i libs/MSL_C++/include \
  -i libs/MSL_C/include -i libs/MetroTRK/include -i libs/RVLFaceLib/include \
  -i libs/RVL_SDK/include -i libs/Runtime/include -i libs/nw4r/include \
  -i build/RMGK01/include -DVERSION=0 -c src/Game/Map/KCollision.cpp \
  -o build/compat-kcollision-farthest/KCollision.o
python3 pc-port/notes/kcollision-farthest-20260903/verify-object.py
```

The included verifier applies ten relocations using the actual callee addresses
and positive-zero constant. It confirms **95/98 retail instructions match
exactly**. The other three retail instructions move the argument, call
`MR::sqrt<float>` at `0x80090e24`, and store its result. The current shared
`MathUtil.hpp` inlines that function, producing a `0x1b8` function instead of
`0x188`. Inspection of the retail square-root callee confirms the same
`frsqrte`/single-precision refinement sequence and zero branch. No source pragmas
or alternate square-root implementation were added to chase this inlining
difference. This is a high instruction match with a documented shared-header
code-generation difference, not a full binary match or native execution test.

Local disassembly and object artifacts are under
`build/compat-kcollision-farthest/`; they are not committed. The verifier is a
snapshot check for this compiler/header state, not a general Game regression.

## PC integration boundary

Whole-TU mirroring is deferred for the integration owner: `Game/xmake.lua` adds
every `**.cpp` automatically. At recovery time PC has neither `KCollision.cpp`
nor `KCollision.hpp`, and lacks `CollisionDirector.hpp` and
`CameraPolygonCodeUtil.hpp` plus their runtime providers. `CollisionCode` and
its `getCameraID(JMapInfoIter)` body already exist.

An exact extraction of the restored body also needs the original-shaped server
header and real providers for `getTriangleNum`, `getPrismData`, `getAttributes`,
`isNearParallelNormal`, and `getPos`, backed by owned decoded KCL and PA data.
The root `KCLFile` pointer/offset unions cannot directly alias big-endian 32-bit
disc headers on a 64-bit host. The current PC `JMapInfo::attach(const void*)`
also deliberately rejects raw data without a sized source. Those resource
adaptations belong in compatibility code. The restored algorithm does not
justify a radius-only shortcut or camera registration over every PA row.
