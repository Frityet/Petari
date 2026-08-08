# FileSelector / FileSelectEffect RMGK02 closure

## Outcome

The remaining retail functions in the RMGK02 FileSelect owner/effect pair are now reconstructed without enabling a PC factory or adding a fallback path:

- `FileSelectEffect::disappear()` preserves the active BRK frame when reversing an appearance, starts a wait-state disappearance from the BRK end, ignores duplicate/dead requests, and kills an effect that is cancelled on its first appearance step.
- `FileSelectEffect::calcAndSetBaseMtx()` builds the retail camera-facing orthonormal basis, including both near-zero rejection checks, and writes that basis plus the actor translation through `MR::setBaseTRMtx`.
- `FileSelector::~FileSelector()` now supplies the retail deleting-destructor symbol.

`FileSelector::exeWaitBind()` and `FileSelectEffect::exeWait()` were already behaviorally present. Their bodies are inlined into their nerve executors in retail; the standalone reconstruction emissions are compiler/symbol-boundary artifacts, not missing gameplay behavior.

## Objdiff evidence

| Unit / symbol | Before | After |
| --- | ---: | ---: |
| `FileSelectEffect` `.text` | 56.8038% | 99.81013% |
| `FileSelectEffect::disappear()` | absent | 99.830505% |
| `FileSelectEffect::calcAndSetBaseMtx()` | absent | 100% |
| `FileSelector` `.text` | 97.30434% | 97.83222% |
| `FileSelector::~FileSelector()` | absent | 100% |

Both final units have zero unmatched retail callable functions. The only mismatch in the newly reconstructed `disappear()` is the relocation identity for the compiler's signed-integer-to-float bias constant: retail references the linker-owned `lbl_80532A40`, while the reconstructed object owns an equivalent local constant. The generated control flow and arithmetic instructions match.

## Exact PC Game mirror

The four root/PC source and header pairs are byte-identical. Current SHA-256 values:

- `FileSelectEffect.cpp`: `e6a7a8091d055256909b43b865cc4daaf57b1930b4f479360b980acadfe9e068`
- `FileSelectEffect.hpp`: `40323e78d6200961c38a580d104213873cf0b77599a19a27f5e884703a7868c1`
- `FileSelector.cpp`: `b24e28534971f37e805d2749a22ed8bf3a257e869bac44aac6ab58c272d60389`
- `FileSelector.hpp`: `abc984fc721f8f58420cbb8a05a0f3497cc1ed3ec9fc74cbd9772c2234d4d363`

The exact PC translation units remain excluded by `pc-port/src/Game/xmake.lua`; this wave did not register `FileSelector`, activate its factory, or introduce an alternate title/file-select owner. After the compatibility layer gained the missing real nerve-query and matrix-basis surfaces, both excluded exact translation units compiled under Clang. FileSelector retained only its pre-existing GCC-compatibility warning for the decomp-style `NO_INLINE` suffix.

## Verification

See `verification.log`. At this lane's capture point, the PC source-mirror suite passed all 32 configured pairs (including all four files from this wave); the integrated foundation gate later expanded that suite and passed 51/51. The full RMGK02 rebuild and DOL SHA check pass, both excluded FileSelect translation units compile, and `git diff --check` is clean for the four owned source files.

## Owned source files

- `src/Game/Map/FileSelectEffect.cpp`
- `src/Game/Map/FileSelector.cpp`
- `pc-port/src/Game/Map/FileSelectEffect.cpp` (byte-identical mirror)
- `pc-port/src/Game/Map/FileSelector.cpp` (byte-identical mirror)

No SaveIcon or TriggerChecker file was touched.
