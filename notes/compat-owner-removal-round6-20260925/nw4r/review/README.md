# NW4R/PPC consolidation review

Read-only review of the round6 canonical owner changes and their consumers. No production edits, build, test execution, staging, or commits were performed. The source-preservation checks were inspected (initially 49, now 50 after the dependency repair); this review adds header/provider and lifetime reasoning.

## Finding resolved during review

**NW4R-001 — resolved build blocker: missing `nw4r::math::MTX34Copy`.** The restored `Pane::LoadMtx` calls `math::MTX34Copy` at `src/nw4r/lyt/lyt_pane.cpp:361`, but the current native `src/nw4r/math/types.h` has only `MTX34Identity` and `MTX34Mult`. Searching `src/` and `aurora/include/` finds no declaration or implementation. The retired provider used `PSMTXCopy` directly, so this is newly required by the donor body. Restore the exact canonical inline from `decomp/libs/nw4r/include/nw4r/math/types.h:85-88`: call `PSMTXCopy(*pIn, *pOut)` and return `pOut`. This keeps the donor Pane implementation intact. Reported to root immediately; root restored this exact helper and the reviewer rechecked its source. Root also resolved the integrated compile's matrix conversion ambiguity by passing `pMtx->m` to `GXLoadPosMtxImm`. This selects the same contiguous 3x4 float matrix and matches the retired provider's argument form. The reviewer made no production edits. There are no remaining high-confidence findings in this bounded review.

## Other reviewed contracts

No other high-confidence regression found within this bounded pass:

- The Font/ResFont split retains unique providers, unchanged class declarations, host-allocated shared state, weak lifetime observation, bounded endian parsing, generation changes, and resource/glyph borrowing. Moving the default Font destructor to its own translation unit is valid: the complete `HostFontResourceState` contains a `shared_ptr` to the forward-declared font type.
- Pane retains its complete virtual surface and one runtime typeinfo provider. The default native constructor remains intact. Both hierarchy mutation guards and the rename guard run before mutation; matrix and animation synchronization retain their former positions. Published native graph retirement still unbinds animation and detaches intrusive child lists before unique ownership destroys any pane. `FindPaneByName` delegates to the same 16-byte comparison. Matrix multiplication delegates to the same `PSMTXConcat`; the Y-axis conversion changes the same three second-column values. The missing copy wrapper identified as NW4R-001 is now restored.
- The restored unqualified string calls remain declared through `common.h` -> NW4R resources/math headers -> the existing native `revolution.h`, which includes `<cstring>`; no new missing string declaration was established.
- NW4R Panic uses the exact allocation scope type previously named by the JKR alias; its host-owned exception behavior is unchanged.
- PPCSync has the same C linkage and fence semantics, with one provider. Xmake and CMake include the new Aurora OS source. `smg-pc-game` already directly depends on `aurora-os`, covering current BloomEffect/J3DCluster callers. No old provider filenames remain in live build registrations.
- Canonical NW4R translation units are explicitly registered. The Mii font fixture uses the original process callback, obtains real archives through DVD, scopes temporary archive copies, and destroys layout/TextBox borrows before the referenced font. The target now carries app/Aurora-main dependencies. Its retail checks still need root's runtime execution.

`review.json` records reviewed file hashes and the exact source-only limits. This review does not claim compilation, runtime success, or complete Wii fidelity.
