# CameraGeneralParam packed-accessor build repair

Timestamp: 2026-08-06T19:15:44Z

## Scope

After the RMGK02 project configuration was restored, the root build stopped while compiling `CamTranslatorSpiral.cpp` because `CameraGeneralParam` no longer declared `getNum1High()`. This repair restores only the two packed `mNum1` accessors used by that assembly-backed translator.

Changed root file:

- `include/Game/Camera/CameraParamChunk.hpp`

No camera algorithm, PC game source, math type, or unrelated failing unit was changed.

## Initial failure

The focused build reproduced the problem with:

```text
ninja build/RMGK02/src/Game/Camera/CamTranslatorSpiral.o

src\Game\Camera\CamTranslatorSpiral.cpp:16:
undefined identifier 'getNum1High'
```

The same source also calls the companion `getNum1Low()`, so both declarations were required even though the compiler stopped after its first diagnostic.

## Assembly-backed API

`CameraGeneralParam::mNum1` is a packed 32-bit parameter at offset `0x30`. The RMGK02 `CamTranslatorSpiral::setParam` target reads its two signed halfwords directly:

```text
lha r6, 0x32(r5)   # value produced by getNum1High()
lha r0, 0x30(r5)   # value produced by getNum1Low()
```

The restored inline accessors preserve those exact big-endian memory halves:

- `getNum1Low()` returns signed halfword index 0, at offset `0x30`.
- `getNum1High()` returns signed halfword index 1, at offset `0x32`.

The `reinterpret_cast` is intentional here because the target API exposes the two in-memory halves of one packed field. No speculative `mNum2` accessors were added; no current source references them and no such API is needed for this build boundary.

## Focused verification

The exact failed target now compiles:

```text
ninja build/RMGK02/src/Game/Camera/CamTranslatorSpiral.o
[1/1] MWCC build/RMGK02/src/Game/Camera/CamTranslatorSpiral.o
```

A one-unit objdiff report compared the generated object with `build/RMGK02/obj/Game/Camera/CamTranslatorSpiral.o`:

- Overall fuzzy match: **100%**.
- Exact functions: **2/2**.
- Exact code: **92/92 bytes**.
- Exact data: **16/16 bytes**.
- `CamTranslatorSpiral::setParam`: **100%**.
- `CamTranslatorSpiral::getCamera`: **100%**.

This confirms both accessor offsets, signed extension, inlining, and call-site code generation.

## Full-build boundary

A subsequent root `ninja` advanced through the camera translator set and into LiveActor and Map units. It stopped at the next unrelated merge/build regression:

```text
FAILED: build/RMGK02/src/Game/Map/FileSelectModel.o

include\Game\Map\FileSelectModel.hpp:27:
struct/union/class member 'FileSelectModel::exeOpen()' redefined
```

`FileSelectModel.hpp` currently repeats the same four declarations—`exeOpen`, `exeBlinkOnce`, `exeClose`, and `exeBlink`—in consecutive blocks. That unit was inspected only to identify the diagnostic and was not edited as part of this repair.
