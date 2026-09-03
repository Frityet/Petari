# Original Mario random-texture update

Recovered `MarioActor::updateRandomTexture(f32)` from verified RMGK01 DOL
`0x802C2BFC`, size `0x110`. The original GC3.0a3 Game compiler produces the same
272-byte function with **99.044% objdiff match**. All 68 instruction opcodes,
direct calls and constant payloads match. The remaining differences are floating
register allocation in the initial probability clamp and local constant labels.

The routine toggles `_B88`, selects `_B80[_B88]->mImage`, and updates exactly
64 bytes in two eight-iteration loops. For each byte it reads the high nibble,
adds four when `MR::getRandom()` is less than `clamp(1 - value / 1000, 0, 1)`,
otherwise subtracts one, clamps the integer to 0 through 15, and writes it back
in the high nibble with a zero low nibble. It finishes with
`DCStoreRange(image, 64)`. It does not allocate or replace textures and does not
sample from the other buffer.

The DOL r2-relative constants are `1000.0f` at `0x806BF80C`, `1.0f` at
`0x806BF7E8`, and `0.0f` at `0x806BF7EC`. Calls resolve to existing
`MR::getRandom` (`0x803E404C`) and `DCStoreRange` (`0x804A5190`), plus original
register save/restore helpers. No missing adjacent texture operation is needed.
Both the float clamp and integer clamp use the existing original MR overloads.

The root TU is parent-owned for a simultaneous rainbow display-list recovery.
This task therefore supplies `root.patch` for the exact placeholder/includes
and `MarioRandomTextureCompat.cpp` with the same native body. No concurrent root
or production write was performed here. `verify-original.py` compiles an ignored
copy of the actual current root TU with this method inserted and records the
full proof in `compiler-evidence.json`; it does not run xmake or native gameplay.

```sh
python3 pc-port/notes/mario-random-texture-20260903/verify-original.py
git apply --check build/mario-random-texture-20260903/root.patch
```
