# MarioTeresa RMGK02 reconstruction evidence

## Scope

- Added the previously absent `src/Game/Player/MarioTeresa.cpp` with every
  retail function represented: 27 gameplay/animation methods, the constructor,
  the local static initializer, and the two simple virtual thunks.
- Expanded `MarioTeresa.hpp` to the assembly-proven `0x5c` layout and complete
  method surface.
- Corrected the assembly-proven `Mario::getHitWallNorm(TVec3f*)` return type to
  `bool` and declared the retail-defined `MarioActor::updateTeresaAnimation()`.
- No PC activation, compatibility bridge, provider, factory, or build-config
  change was made.

## Focused objdiff

```text
ninja build/RMGK02/src/Game/Player/MarioTeresa.o
build/tools/objdiff-cli diff -p . -u main/Game/Player/MarioTeresa \
  -o pc-port/notes/mario-teresa-rmgk02-20260809T050110Z/focused-objdiff.json \
  --format json-pretty
```

- `.text`: 75.5819% across all 9,060 retail bytes
- `.ctors`: 100%
- `.data`: 63.797028%; the 72-byte vtable is exact, `teresaAnimeTable` is 95%,
  and `teresaAnime2` is 66.667%
- `.sdata2`: 92.42424%
- 10 exact methods, including both state predicates, both state thunks,
  `checkAccel`, `resetTeresaMode`, and the Mario reflection forwarder
- Major behavior bodies are high fuzzy matches: horizontal velocity 99.875%,
  disappearance 99.309%, drop-flag update 97.262%, Teresa reflection 96.18%,
  state update 96.261%, and start 90.116%.

The remaining differences are source-expression, string-pooling, constructor,
and animation-controller code-shape differences. No marginal register or
relocation tuning was performed.

## Full verification

```text
ninja -j12 build/RMGK02/main.dol
build/tools/dtk shasum -c config/RMGK02/build.sha1
git diff --check
```

The full DOL check reports `build/RMGK02/main.dol: OK` with SHA-1
`54b71431af0d509097bfdef4ec28617afc487e89`.

## PC provider implication

The reconstruction introduces no new external provider requirement. Future PC
activation still needs an explicit pointer-width/layout decision for the retail
`MarioActor::_9B8` and `_9BC` 32-bit animation pointer slots; this lane leaves
the retail ABI and all PC activation code untouched.
