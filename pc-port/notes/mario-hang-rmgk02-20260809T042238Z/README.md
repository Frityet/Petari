# MarioHang RMGK02 reconstruction evidence

## Scope

- Reconstructed the source-absent `src/Game/Player/MarioHang.cpp` from the
  RMGK02 object/assembly, with all 16 named retail functions and the static
  nerve initializer represented by C++ source. No inline assembly is used.
- Completed the dedicated `MarioHang.hpp` virtual surface and layout. Retail
  constructor stores and member accesses prove a 32-bit size of `0x44`; the
  retail vtable is 72 bytes and matches at 100%.
- Corrected the assembly-proven shared declaration of `Mario::fixHangDir` from
  `void` to `bool`; both retail callers branch on its return value.
- No PC activation, provider, compatibility, protected-file, or
  `configure.py` edits were made. MarioHang introduces no new PC provider
  requirement.

## Focused RMGK02 objdiff

Command:

```text
build/tools/objdiff-cli diff -p . -u main/Game/Player/MarioHang \
  -o /tmp/mariohang-diff.json --format json-pretty
```

- `.text`: 85.85462% across 9,300 retail bytes
- `.ctors`: 100%
- `.data`: 100% (including the 72-byte `MarioHang` vtable)
- `.sdata2`: 93.333336%

Exact functions are `Mario::isHanging`, `MarioHang::recordWallPolygon`,
`recordHangNorm`, `forceDrop`, the constructor, and the static initializer.
Notable large-body scores are `Mario::checkHang` 90.48291%,
`MarioHang::update` 69.730934%, and `MarioHang::tryClimb` 83.29082%. The
remaining differences are compiler expression/register/stack shape; the
retail branches and functional state transitions are present.

## Verification

```text
ninja build/RMGK02/src/Game/Player/MarioHang.o
ninja -j12 build/RMGK02/main.dol
build/tools/dtk shasum -c config/RMGK02/build.sha1
git diff --check
```

All commands passed. Rebuilt and retail DOL hashes are identical:

```text
SHA-1   54b71431af0d509097bfdef4ec28617afc487e89
SHA-256 8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf
```

`ghidra-pseudocode.txt` is an auxiliary cross-check retained in this ignored
directory; RMGK02 assembly and the focused object diff remain authoritative.
