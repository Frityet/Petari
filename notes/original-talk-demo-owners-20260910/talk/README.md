# Original talk-owner prerequisite: MessageHolder and native BMG storage

Scope: activate the complete original `MessageHolder.cpp` translation unit and the real system/game message owner that TalkDirector/TalkNodeCtrl require. This does **not** activate TalkDirector or replace the remaining host TalkRuntime graph. No GameSystemObjHolder is fabricated.

## Current source closure

Fresh LLVM 23 syntax probes are recorded in `current-native-probes.json` and `*.current-native.log`. Missing original Game headers were staged only in this notes directory for the probe, preserving native SDK declarations first. Current direct findings:

- TalkTextFormer and TalkSupportPlayerWatcher compile.
- TalkDirector stops at missing `revolution/gx/GXGet.h`.
- TalkBalloon has local fmin/fmax conflicts with libc++; TalkState has integer/nullptr comparisons; TalkMessageCtrl has bool/nullptr comparisons and a missing stage-state query declaration.
- TalkNodeCtrl stops at the original tag-processor declaration closure.
- DrawSyncManager needs actual GX FIFO/breakpoint surface and its reference constructor/callback/thread bodies are absent. Importing its existing few bodies cannot create a complete callback owner.
- MessageHolder reference bodies are complete. Its former full GameSystemObjHolder include opened unrelated ownership headers; the new native seam accesses an explicit complete MessageHolder instead.

These are first direct blockers, not a claim that the complete TalkDirector dependency closure links or runs.

## Original code and explicit native boundaries

`src/Game/System/MessageHolder.cpp/.hpp` copy the full reference TU/declarations. All original message index lookup, information field reads, node lookup, branch methods, scene alias transitions and owner initialization remain. Native differences are limited to:

1. A `TARGET_PC` constructor step converts bounded retained BMG bytes into native scalar order and native wchar_t-width storage.
2. A trailing native storage owner retains the fields borrowed by original methods; its destructor releases original JMapInfo metadata.
3. Four MessageSystem methods obtain the scoped actual MessageHolder rather than dereferencing an unavailable GameSystemObjHolder. Direct ownerless access fails explicitly.
4. The exact original SDK `QUESTIONMARK_MAGIC` constant is restored to the native JKRArchive declaration.

No reference decomp source change was needed. No Game control flow or message-state algorithm was reimplemented.

`NativeBmgResource` is a resource-format adapter, separate from the Game owner. INF1 scalar fields and text byte offsets become native; DAT1 retains one wchar_t for each original UTF-16 code unit. Tag payload zeroes, surrogate units, terminators, shared string offsets and interior offsets remain intact. FLW1 nodes preserve their union distinction: event nodes contain one u32 argument while the other node forms expose two u16 fields. Branch sentinels remain 0xffff. FLI1 and unknown payloads are retained opaquely because current original MessageData only exposes their common block pointer. This does not claim future direct FLI payload readers are native-ready.

`MessageHolderOwnership` owns actual MessageHolder plus both actual MessageData objects in an explicit original Game heap. The raw native buffers, archive bookkeeping and parser metadata belong to host allocation scopes. The process GameResourceRuntime budget supplies the domain; RuntimeContext supplies the disc/mount lifetime. System data comes from the exact selected language's System.arc inside original immutable ErrorArchive. Game data comes from the selected real localized Message.arc and is published under the original `/MessageData/Message.arc` alias. Current RuntimeContext retains its existing RMGK01/Korean language selection; this is not an automatic language-selection feature. Scene data remains the original explicit game-data alias controlled by `initSceneData`/`destroySceneData`; no scene initialization is invented by process construction.

MR system/game/layout getters now use the actual records and keep their existing null absence contract. Native UTF-16 access uses the same authoritative record index. Pointer-to-ID metadata is used only for native layout presentation. The host MessageService still exists for host diagnostics/formatting; it no longer substitutes synthetic messages into the original getters. Its existing presence predicate semantics are retained; this change does not claim recovery of the reference tag-aware `isExistGameMessage` length predicate.

## Archive path finding and correction

The first root-linked run reached the new owner and failed on the mixed-case embedded system path. Original JKR CArcName folds path components, while native RarcArchive resource resolution previously tried exact path followed by basename-only fallback. This could select a sibling locale's System.arc. `RarcArchive::find_resource` now tries the complete case-folded ASCII path before basename lookup. A qualified path that is missing stays absent; a basename-only resource query keeps its existing lookup behavior. The owner performs actual JKRArchive resource lookup. The fixture verifies the Korean full-path identity and compares the decompressed selected source bytes to the mounted system archive; it also rejects a missing locale.

## Validation status

- Isolated native syntax succeeds for MessageHolder, NativeBmgResource, MessageHolderOwnership, MessageUtilCompat, RuntimeContext and RarcArchive. Logs are `*.typed-native.log`.
- New `tests/OriginalMessageHolderTests.cpp` syntax succeeds. It requires `SMGPC_REAL_DISC`, rather than skipping retail checks. It exercises actual MessageData construction from a well-formed synthetic RARC/BCSV/BMG (raw CP932 identifiers versus UTF-8 spelling; camera and signed metadata; embedded tag zero; surrogate units; aliased/interior text offsets; event union and branch sentinel), malformed byte offsets/branch range rejection, every authored system/game record and flow entry, public getters, explicit scene aliases, duplicate-owner rejection, and two complete owner generations with metadata/mount/original heap retirement checks.
- `OriginalStarPointerOwnerTests` now tests retained guidance pointers using real System_Date000/System_Time002 data, comparing all raw units instead of injecting synthetic host messages.
- Root linked and ran the complete MessageHolder fixture successfully on the supplied RMGK01 disc: all 6 system + 1,994 game records and flow entries, both owner generations, raw native boundary checks, utility queries and clean teardown passed. See `../run-message-holder-tests.log.gz`.
- The complete StarPointer owner fixture passed with actual sampled WPad ownership and actual BMG retained pointers. See `../run-star-pointer-owner-tests.log.gz`. Its stale fixture formerly changed only the SDK connection flag; it now calls the existing owner's update path, which dispatches SDK callbacks and runs original WPadHolder sampling before reading Game connection state.
- Root's initial combined showcase replay completed 960 ticks with all 13 movement/camera checks, both jump/landing cycles and clean exit at 60.007 presentation FPS. Binary `7b437fcf57eabc737234a3ebc4f03a873066067752d85e340141721dc7a73cc0`; see `../replay-summary.json` and `../replay-validation.json`. This is bounded Mario demo regression evidence, not full TalkDirector/bunny-chase completion.
- The final duplicate typed-block rejection was added after that initial runtime proof. It prevents first-block parsing and retained native DAT1 pointer selection from disagreeing on malformed duplicate INF1/DAT1/FLW1/FLI1 identities. Its appended valid DAT1 regression and all final sources syntax-check successfully. Root rebuilt and reran the final MessageHolder target: PASS, including all 2,000 authored records and two generations. The original JKRArchive regression also passed all 6 groups. Root is performing the final combined movement replay separately; no final replay result is inferred here.

The current original TalkDirector migration frontier is unchanged: the real message prerequisite is now implemented, but balloon/state/tag/Drawing callback ownership remains work for later bounded cohorts.
