# Shared resource scalar decoding — 2026-09-12

This cleanup removes 31 private endian reader functions (167 lines of definitions) from 12 resource, layout and SDK adapter files. The initial scalar-only cleanup covered 11 files; the subsequent contiguous UTF-16 BMG fix removed two more readers from `NativeBmgResource.cpp`. The implementation now uses the existing `aurora::endian::read_big` primitive instead of maintaining separate byte-shift code for RARC, BCSV, BMG, BRFNT, BRLYT, TPL/BTI, Yaz0, J3D model and transform resources, JKR archives, and NW4R fonts. The per-file list is in `removed-readers.json`.

## Shared boundary

`aurora/include/aurora/endian.hpp` already provided unsigned, unaligned big-endian reads for Wii resources and GX command streams. It now supports signed integer and floating-point scalar representations of 1, 2, 4 and 8 bytes, and adds a checked byte-span overload. The original unsigned pointer reader, writers and packed `BigEndian` object remain available with the same results.

The reader assembles an unsigned integer of the exact width and bit-casts to the requested type. It performs no floating-point arithmetic and preserves negative zero, subnormals, infinities, and NaN payload bits. The span overload checks `offset > size || width > size - offset`; it cannot accept an invalid position by wrapping `offset + width`. It throws a host-owned `std::runtime_error` through Aurora's existing exception allocation boundary. Resource-specific structural checks remain in the parsers; this is not a complete malformed-resource arithmetic audit.

`src/Game/System/LayoutHolder.cpp` uses the existing `read_u32` primitive for its one native architecture correction: original `GetResource` loads the big-endian word at byte offset 4. The offset, table selection and original control flow are unchanged. The decomp reference retains the original native-word load; no new decomp change is needed for this cleanup.

## Callsite review

`callsite-equivalence.json` records a normalized comparison of the initial 11 scalar-only parser/SDK files against HEAD. All 11 agree after removing the duplicated helper definitions and replacing the function names with their explicit scalar type. The only additional equivalent expressions are:

- RARC's 24-bit name offset reads the original packed flags/name word at offset 4 and masks its low 24 bits. The prior reader read bytes 5–7. Both use the same complete, bounds-checked file-entry span.
- J3D transform value arrays load their declared scalar type directly, replacing the old size branch and explicit bit-cast. Signed frame counts use the same operation.

The raw SDK font header accessor still has the original valid-pointer precondition; its API provides no byte-span length. JKR archive structure validation still uses its existing `invalid_argument` checks. Any failure specifically at the shared scalar-span boundary now has the common `runtime_error` message rather than a parser-specific message. No resource ownership, caching, paths, archive mounting or actor behavior changes in this cleanup.

## Validation

The standalone `aurora/tests/endian_test.cpp` is registered as CTest `endian`. A direct LLVM 23 build with AddressSanitizer and UndefinedBehaviorSanitizer passed all 92 checks, with no sanitizer reports. Coverage includes every supported signed/unsigned width, unaligned input, the existing pointer/writer/packed-object entry points, exact float and double bit patterns, the final complete scalar, a scalar truncated by one byte, an empty span, and offsets near `SIZE_MAX` that previously could wrap an addition-based check.

Build command (from repository root):

```sh
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I aurora/include aurora/tests/endian_test.cpp -o notes/compat-resource-consolidation-20260912/endian-test
notes/compat-resource-consolidation-20260912/endian-test
```

Receipts: `endian-test.build.log` and `endian-test.run.log`. The generated executable is not source evidence and should not be committed.

Parent integration fixture runs are pending at this note's first write. Relevant existing fixtures are `smg-pc-original-resource-holder-tests` (original holder and JKR/RARC boundaries), `smg-pc-original-message-holder-tests` (BMG and BCSV), `smg-pc-picture-font-tag-tests` (retail BRFNT, BRLYT and TPL), and `smg-pc-original-j3d-transform-animation-tests` (typed original animation values). This scalar cleanup does not establish process startup or Gateway gameplay completion.

## Remaining structural opportunities

Model resources still have a native typed-resource owner distinct from the raw layout owner. Consolidating their JKR archive lifetime requires preserving typed J3D resource registration, native allocations and externally retained original objects together. This patch does not add another facade or claim that larger ownership consolidation is complete.

## Review follow-up

The subsequent contiguous UTF-16 storage correction and independent FileSelect/MarioSound review are recorded in [UTF16.md](UTF16.md).

Coordinator integration: original-resource-holder, picture-font-tag, original-layout-group, original-message-holder (both complete real-disc lifetimes) and file-select-name tests pass. The complete source also links and completes a 240-tick Gateway smoke. See ../gateway-integration-20260912/ for final receipts.
