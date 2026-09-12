# Preserve complete UTF-16 DAT1 storage — 2026-09-12


Reviewing the restoration of `FileSelectFunc.cpp` exposed a pre-existing resource-boundary error. `NativeBmgResource::message_utf16` returned a separate `std::u16string` for each message, while original fixed-size consumers can copy through a terminator into the following authored DAT1 words. Separate strings also lost original equal-offset and interior-offset pointer identity. The native `wchar_t` view already preserved that full-block identity.

`NativeBmgResource` now keeps a contiguous, native-endian `std::uint16_t` vector for the complete original DAT1 payload. UTF-16 getters use the same original message offsets as the wide view and retain aliases, interior pointers, embedded control-tag zeros, surrogate units and the authored words after a short terminator. The native resource getter now returns the same `std::uint16_t` type used by its consumers, removing the two `char16_t`/`u16` pointer reinterpretations. No name-specific padding or extra terminators are added. The parsed `BmgMessageArchive` is now constructor-local rather than retaining its separate per-message strings and lookup maps for the entire resource lifetime.

`OriginalMessageHolderTests.cpp` now checks UTF-16 aliases/interior offsets and embedded tag units with its existing synthetic BMG. Its five fellow-icon checks independently decode all eleven copied words from the real archived BMG's original DAT1 offsets and check both source bounds and destination guard words. They no longer use the same UTF-16 getter as their expected-data oracle. These integration edits await the coordinator's rerun.

The same test now supplies a retained original `MessageFont26.brfnt` resource from the existing DVD layout-archive resolver to the original message-tag processor: the original tab/rectangle path requires font metrics. Its former null-message-ID expectation was removed because the restored original lookup requires a valid string. These are fixture corrections; no new Game fallback was introduced.

The read-only MarioSound review confirmed that the former compatibility provider's text is preserved exactly before the source's original trailing nerve declarations. Those declarations use the established native `INIT_NERVE` macro, which emits no duplicate instance definitions. Typed sound-swap columns preserve original indexing for columns 1–3, and numeric high-byte flag extraction preserves original big-endian union-byte meaning. Game-root execution-character-set coverage now handles the source without a separate compatibility-file admission.

## Final alignment, offset and lifetime review

The `vector<std::uint16_t>` provides the required code-unit alignment and remains stable after construction. The constructor already rejects odd original offsets and partial DAT1 units, validates messages and control tags through the existing BMG parser, and checks representability of relocated native offsets. Dividing the relocated offset by `sizeof(wchar_t)` recovers the original UTF-16 element index for both supported host widths. The getter checks the message-table index before publishing a pointer.

This change preserves the complete DAT1 payload accepted by the existing parser; it does not add a buffer length to raw getter APIs or promise arbitrary fixed-length reads are safe. Consumers retain their original resource-validity preconditions. The eleven-word source extent is checked explicitly for the five authored fixture messages.

Construction and destruction continue under the existing host-allocation scopes. No member points into the constructor-local parser; persistent storage consists of native block bytes, complete UTF-16 units and the relocated offset table. Both borrowed views remain valid only while their `NativeBmgResource` owner remains alive.

The follow-up also removes this file's two private `be16`/`be32` scalar readers and uses the same existing Aurora primitive. Structure-level `require_range` checks remain. Together with the initial eleven-file pass, 31 private readers (167 definition lines) are removed from twelve files.

Coordinator integration: original-resource-holder, picture-font-tag, original-layout-group, original-message-holder (both complete real-disc lifetimes) and file-select-name tests pass. The complete source also links and completes a 240-tick Gateway smoke. See ../gateway-integration-20260912/ for final receipts.
