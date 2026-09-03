# Original camera compilation closure

This package closes the six remaining native compilation failures in the
original Camera directory. It prepares the actual camera owner integration;
it does not activate CameraDirector or claim a complete runtime camera system.
Production native files were left untouched. Root source corrections and the
staged native overlay are ready for the parent integration.

## Changes and original semantics

- `CameraContext`: copy the complete original `TMatrix44::concat` and its
  `SMatrix44C` row-pointer conversions into the native SDK header. The original
  16 local results preserve both left and right operand aliasing.
- `CameraTestObj`: copy original `TRotation3::identity33`, which writes only
  the rotation block and retains translation.
- `DotCamReaderInBin::hasMoreChunk`: use the existing `JMapInfo::getNumEntries`
  API instead of assuming its storage is a raw `JMapData*`. This is identical
  under the original compiler and permits the genuine native JMap owner.
- `CameraRailHolder` and `CameraCover`: expose the existing original
  `getJMapInfoRailArg0NoInit` and `joinToNameObjGroup` declarations. Include the
  original scenario-opening state declaration used by CameraDirector as well.
  These are declarations only; owner/provider activation remains separate.
- `GameCameraCreator`: preserve its original source and supply native `mem.h`
  as a general forwarding header to `<cstring>`. The root MSL `<cstring>` does
  **not** declare `memset`, so removing `<mem.h>` fails the original compiler.

`CameraManGame::createStartAnimCamera` stores a real animation data pointer in
`CameraGeneralParam::mNum1`; both actual `CamTranslatorAnim` methods read it.
The field therefore becomes `intptr_t`, with the pointer write using the same
type. The unchanged original assignment operator copies its entire value.
There is no separate pointer side channel. Native resource decoding still
reads an `s32`, sign-extends into the field, and preserves its previous value
when `num1` is absent. Numeric consumers keep original signed values.

The original packed accessors address two signed halfwords in big-endian
memory order. Their native implementations explicitly extract bits 31–16
and 15–0 respectively; `CamTranslatorSpiral` uses those existing accessors.
Their historical `Low`/`High` names do not describe little-endian ordering.

The staged matrix header also carries the coordinated collision package's
`getScale` declaration and `TPosition3(pointer)` constructor. It preserves the
already committed `setEulerZ`; its implementation is not replaced here.

## Verification

`python3 pc-port/notes/original-camera-compile-closure-20260903/verify-original.py`
passes with the original GC 3.0a3 compiler. Seven relevant functions retain
identical bytes **and relocations**, totaling 1,660 bytes:

| Function | Unchanged bytes |
| --- | ---: |
| DotCamReaderInBin::hasMoreChunk | 164 |
| CameraManGame::createStartAnimCamera | 164 |
| CameraGeneralParam::operator= | 144 |
| CameraParamChunk::load | 1,072 |
| CamTranslatorAnim::setParam | 20 |
| CamTranslatorAnim::getAnimFrame | 12 |
| CamTranslatorSpiral::setParam | 84 |

The compiler also verifies original `sizeof(CameraGeneralParam) == 0x3C`,
`sizeof(intptr_t) == 4`, and field offsets `0x30`, `0x34`, and `0x38`.
`compiler-evidence.json` records each code hash and relocation count.

All 127 original Camera translation units compile with Homebrew LLVM 23,
native ARM64/TARGET_PC flags, this overlay, and the parent's camera-owner
overlay. `native-commands.json` records the complete actual commands. This
checks compilation only; it does not establish that every referenced provider
links or that the complete original director runs.

The isolated native fixture passes real pointer storage/copy/readback above
32-bit address range, signed numeric extrema, signed packed halfwords,
independently expected projection matrices with either input aliased, and
rotation-only identity preserving translation. It links the complete original
parameter assignment body and real CameraParamString implementation. No fake
camera, resource holder, or production callback is used. It does not execute
the complete DotCam/CameraHolder resource-load lifecycle.

## Applying and reproducing

- `native-headers.patch`: SDK helpers, declarations, native parameter header,
  and general `mem.h` forwarding.
- `native-sources.patch`: complete original camera sources required by these
  corrections, mirrored from root. Applying these creates auto-globbed Game
  translation units; defer to the coordinated complete camera activation.
- `native.patch`: the combined patch; `native-manifest.json` records its
  preimage commit and file hashes.
- `stage-native.py`: reconstructs only the ignored overlay from that commit
  and patch, without changing production files.
- `verify-native.py`: repeats the captured compile commands and boundary
  fixture. It requires the existing parent camera-owner overlay and the
  recorded native toolchain/package paths.

Root source for this checkpoint is limited to CameraParamChunk.hpp,
CameraParamChunk.cpp, CameraManGame.cpp, CamTranslatorSpiral.cpp, and
DotCamParams.cpp. No Game algorithm or gameplay selection was substituted.
