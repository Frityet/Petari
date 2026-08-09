# MarioWalk RMGK02 reconstruction

Date: 2026-08-09 UTC

## Outcome

`src/Game/Player/MarioWalk.cpp` now contains source reconstructions for all 14 retail
functions in the RMGK02 `MarioWalk.cpp` unit. The implementation covers the walk
speed tiers, crouch walking, walk/idle blending, slope animation, braking, wall
push, moving-platform press handling, ice movement, sand sinking, poison damage,
and shallow-water state/effects.

The final focused object is 94.33228% matched in `.text` against the retail unit.
The complete RMGK02 build succeeds and `build/RMGK02/main.dol` passes the configured
retail SHA-1 check.

No PC activation, compatibility, factory, staging, audio-subsystem, SaveIcon, or
TriggerChecker files were changed for this reconstruction.

## Authoritative evidence

- Retail assembly: `build/RMGK02/asm/Game/Player/MarioWalk.s`
- Retail address range: `0x80306B68..0x80308E04` (`0x229C` bytes)
- Focused object: `build/RMGK02/src/Game/Player/MarioWalk.o`
- Objdiff unit: `main/Game/Player/MarioWalk`
- Full-build checksum manifest: `config/RMGK02/build.sha1`

The tables, constants, branches, calls, member offsets, animation/effect names, and
control-flow decisions in the source were reconstructed from the retail assembly.
Objdiff was used after every material source-structure adjustment.

## ABI corrections

Two size-neutral declarations in `include/Game/Player/Mario.hpp` were corrected:

1. `Mario::checkWallPush()` is `void`, not `bool`. Retail function
   `0x80307F68..0x80308104` has no returned value. Its callers at `0x80307D78` in
   `MarioWalk.s` and `0x802CBAAC` in `MarioAnimator.s` both ignore `r3` after the
   call.
2. `Mario::_71C` at offset `0x71C` is a `u8` walk-speed tier, not a `bool`. Retail
   `decideWalkSpeed()` uses `lbz`/`stb`, assigns tiers `6`, `4`, and `3` at
   `0x80307478`, `0x8030749C`, and `0x803074BC`, and indexes eight-entry walking
   tables with the byte. Keeping the field as `bool` made MWCC collapse every
   nonzero assignment to `1`, which was functionally incorrect. `u8` preserves the
   existing size, offset, and surrounding layout.

All unrelated shared-header edits present in the worktree were preserved.

## Secondary and ignored evidence

- `ghidra_decompile.py` and `ghidra-pseudocode.txt` record a no-analysis Ghidra
  discovery pass against `build/RMGK02/main.elf`. Ghidra recovered a few useful
  address/field-access hints, but reported bad instruction data for most large
  functions and inferred several incorrect prototypes. Those incomplete bodies
  were ignored as reconstruction authority.
- A temporary m2c pass (`/tmp/MarioWalk.m2c.cpp`) helped label stack temporaries and
  branch regions. Its inferred structures, prototypes, and register-derived call
  arguments were not trusted and were not copied into the final source.
- The retail assembly and MWCC/objdiff results remained authoritative whenever a
  discovery tool disagreed or was incomplete.

## Remaining nonmatching codegen

The residual mismatch is code-generation shape, register allocation, local static
data/relocation identity, and the generated constructor-section association. No
retail behavior is intentionally omitted. The largest remaining function-level
gaps are `decideWalkAnimation()` (93.1157%) and `updateWalkSpeed()` (94.05937%);
all other functions are at least 94.11%, with most above 96%.

See `objdiff-summary.txt` and `verification.log` for exact metrics and commands.
