# Active GX miscellaneous state and MSL adapters

Aurora now supplies actual GXSetMisc state semantics, with the original u16
vertex-count truncation, conditional VCD dirty bit, display-list context flag,
and abort-copyout setting/default. Both primitive-flush guards now test the
positive vertex count and pending-BP flag explicitly. This replaces a native
integer alias of adjacent halfwords and matches the original SDK's conjunction
of complementary flags. The missing aurora-gx → aurora-os build dependency is
also explicit, so independent GX clients resolve the actual MEM1 boundaries.

The functional shim supplies mem_fun_ref and its unary-negation composition on
the existing generic member adapter. This lets original std::find_if calls use
reference identity and const/noexcept members with the modern C++ library.

Normal build/run checks passed:

- `smg-pc-gx-misc-state-tests`: 42 state checks, all flush-flag combinations,
  actual display-list save/restore, no GPU initialization or Game dependency.
- `smg-pc-legacy-functional-adapters-tests`: original algorithm composition,
  mutable reference identity, const/noexcept members, empty/exhausted ranges.
- Existing `smg-pc-msl-functional-tests`: passed unchanged.

The separate `original-effect-system-native-20260903` package preserves original
compiler/disassembly evidence and independently instrumented ASan/UBSan tests.
Its six-file Aurora patch is now applied; the extra build-dependency correction
is part of this production checkpoint. The effect cohort and original actor
EffectKeeper activation remain staged. No Game source is changed here.

Token 3 retains its real setting; GXAbortFrame still lacks its native consumer.
These tests do not claim frame-abort support or rendered particle output.
