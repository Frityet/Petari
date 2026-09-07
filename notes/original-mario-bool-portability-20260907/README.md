# Mario boolean comparison portability

Two original `MR::normalizeOrZero` boolean-result comparisons used `nullptr`, accepted by the Wii compiler but rejected by native LLVM. Replaced only those operands with `false`, preserving the original zero/nonzero predicates.

Both full Mario.cpp translation units freshly compile with the current Wii compiler and headers. Baseline-to-restored object comparison pairs 110 symbols: all 110 remain 100%. This proves the edits leave the generated Wii implementation unchanged; it does not claim all Mario routines already match retail. No native source changed in this checkpoint.

Only production path: `decomp/src/Game/Player/Mario.cpp`. Parent coordinates commit and native mirroring.
