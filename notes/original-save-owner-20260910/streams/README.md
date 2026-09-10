# Original JSU streams and native memory providers

Status: FROZEN. Native LLVM 23 compile/link exit0 and linked runtime exit0, 12/12 test groups, AddressSanitizer and UndefinedBehaviorSanitizer enabled with no diagnostics. Exact commands and binary digest are in `native-results.json`; compiler and execution output are in `native-build.log` and `native-tests.log`. No Xmake or commits were performed by this task.

## Implemented surface

Restored the JSUIosBase, input/output, random-access input/output, and memory input/output hierarchy under `src/JSystem/JSupport/`, with concrete implementations in `src/compat/JSUStreamCompat.cpp`. These restore original virtual dispatch, polymorphic destruction, partial transfers, state transitions, buffer replacement, and both kinds of skip. The original existing memory-only header facades are replaced rather than retained alongside the hierarchy.

Raw read/write/readData/writeData perform raw byte copies. There is no byte swapping or save-format recognition here. Typed U8/U16/U32 helpers read/write host-native scalar bytes. Per root coordination, native input helpers initialize only their otherwise-indeterminate local temporary to zero before the ordinary partial read; this defines an EOF return without replacing partial transfers with all-or-nothing behavior. An explicit save container adapter owns serialized endianness outside these streams.

Raw reads copy only the available prefix and preserve every untouched destination byte. EOF reads leave persistent caller locals unchanged. This supports the separately recovered original VLE1 reader's persistent hash/value locals; the generic stream has no per-game last-value cache and adds no EOF padding.

## Retail state proof and reference correction

The linked game library really uses a bool state. This is not normalized to a textbook bitmask. `JSUIosBase::setState` at8041741C loads the state byte, ORs the requested state, then explicitly normalizes the result with subic/subfe before storing one byte. Thus setting IO_MEMORY_ERROR(2) produces stored true(1), and querying bit2 does not recover it. The native implementation preserves this demonstrated quirk.

`retail-state-proof.json` independently compares the relevant saved assembly bytes against actual original `decomp/build/compat-math-oracle/main.dol`, including both input/output seek clear-state sequences. The DOL SHA1 is25c5959534b3c21246c6c7e42021b916b41fb578.

The only decomp edit is `decomp/libs/JSystem/include/JSystem/JSupport/JSUIosBase.hpp`: `clearState` changes `mState &= state` to `mState &= ~state`. Output seek at8041753C clears bit0 before bool normalization, so the old header retained the very error it was intended to clear. Reference was changed first, after baseline Wii compilation; native imported that corrected header.

The full original JSUOutputStream TU compiles before and after with MWCC GC/3.0a3. For the direct seek comparison, the same compiler options plus `-ipa file` expose the inline helper at its original call site: original-header seek94.47369%, corrected-header seek100%. `wii-results.json`, `before-ipa-objdiff.json` and `fixed-ipa-objdiff.json` record commands and matches. The normal configured compile is also retained; it keeps the helper out of line and therefore cannot distinguish its change in the seek function's own score. No source change was made just to raise a matching score.

## Observable contracts

- Public read/write set IO_ERROR when the returned count differs from the requested count. Successful transfers do not clear a previously set error.
- Direct readData/writeData do not set the wrapper's IO_ERROR themselves.
- A random seek returns actual displacement, clamps its destination, and clears IO_ERROR even if clamped. `SEEK_FROM_END` means length-minus-offset, not POSIX end-plus-offset.
- Random input skip uses seekPos and sets IO_ERROR if clamping prevents the requested displacement. Negative random input skip can rewind.
- Sequential input skip consumes bytes through virtual readData; output skip emits the given fill byte through virtual writeData, stopping at the first short transfer. Negative sequential skip performs no transfers.
- setBuffer resets position and records the new buffer/length without clearing an existing error state.
- The streams borrow their memory; virtual destruction does not free caller buffers.

The host memory provider uses byte-pointer arithmetic without truncating a pointer to 32 bits. Seek arithmetic widens to64bits before addition/subtraction and clamps before returning an SDK s32 position/displacement, avoiding signed overflow on extreme offsets. Negative native buffer lengths remain clamped to zero as in the prior host memory facade; null buffers/arguments and nonpositive transfer lengths perform no byte access. The public wrapper marks a positive/negative nonzero request returning0 as an error. These invalid-span protections define native behavior outside the original valid-buffer contract.

## Validation scope

`tests/OriginalJSUStreamTests.cpp` exercises actual restored providers and includes twelve groups: retail bool state, partial raw input, partial raw output with guards, host-native typed roundtrips, typed temporary versus persistent raw EOF, all seek origins/displacement/clamping, input/output memory skip, base-class virtual sequential skips, sticky-state buffer replacement, invalid native spans, extreme signed seek offsets, and polymorphic destruction. The sequential test types use the production memory provider as their actual backing store and count virtual byte dispatches.

The sanitizer run proves the focused native memory/API paths, not complete save-file ownership or persistence. Root owns the save-container adapter, original chunk activation, linked full-port builds, and publication. `source-manifest.json` records all nine native paths plus the single reference header correction.
