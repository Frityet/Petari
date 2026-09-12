# Original string/message source consolidation

## Final source ownership

The complete recovered `Game/Util/StringUtil.cpp` and `Game/Util/MessageUtil.cpp` translation units now replace five duplicate compatibility providers: `OriginalArchiveString.cpp`, `StringComparisonCompat.cpp`, `StringUtilCompat.cpp`, `OriginalStringScan.cpp`, and `OriginalMessageLineQueries.cpp`. `MessageUtilCompat.cpp` keeps only the explicit fixed-width UTF-16 accessors and the retained layout-message identity boundary. Its duplicate original direct getters and existence predicate are removed. `TalkCompat.cpp` no longer owns a separate galaxy-name formatting function. The unmodified original `Game/Map/RaceManager.hpp` supplies MessageUtil's missing declaration header. Parent owns Xmake exclusion removal.

This activates all currently recovered functions from those sources, not every retail function: StringUtil's variadic number-tag formatter remains undecompiled, as do several MessageUtil character/figure/line extraction methods. Previously uncalled Race/ReplaceTag/selected-scenario dependencies remain real future link frontiers; no substitute definitions were added for them.

## Reference corrections and native width

- `getBasename` now returns the original path when `strrchr` finds no slash. Assigning the final component only for a non-null separator and then returning the path gives a natural 93.52941% match, replacing the earlier incorrect null result. The retail assembly preserves the input in r31 and returns it for a missing separator.
- `getStringLengthWithMessageTag` advances by the encoded tag byte length divided by two. The previous decomp treated byte length as a wchar count. Retail instructions explicitly clear the low size bit for pointer advancement and divide the size by two for the returned count. The corrected 132-byte function matches 99.393936%.
- The already-proven unsigned address comparison in extension removal returns to its actual source owner. The original valid-path contract is retained; no absent-extension fallback is invented.
- The wcsncpy declaration now agrees with its real pointer return type. Native code includes host string/wide-character headers; Wii-only va_list declarations remain on Wii.
- BMG storage keeps original UTF-16 code-unit values in widened native wchar_t slots. Native picture-tag writes and byte conversion therefore operate on word values rather than interpreting the host byte layout. Tag group/length reads reuse the existing MessageEditorMessageTag parser. Picture glyph values explicitly truncate to u16, matching retail sth stores. The shared parser still interprets tag sizes in original encoded bytes.
- MessageUtil's integer length comparison uses zero instead of a pointer literal. Its original behavior is now visible: an existing empty first page is not an existing game message, while embedded tag payloads do not terminate the scan.

## Owner behavior

The removed direct-message wrapper previously converted a missing MessageHolder into nullptr. The original source calls MessageSystem, which deliberately requires the active real MessageHolder. Missing identifiers inside a valid holder still return nullptr because TalkMessageInfo starts with a null message pointer; an absent owner now raises the existing explicit error. The fixed-width UTF-16 API is unchanged for FileSelect/RFL and save-banner consumers.

## Proof and tests

`message-string-match-summary.json` and the compressed objdiff reports cover 35 recovered StringUtil retail methods and 15 comparable MessageUtil methods. Of StringUtil's 35 methods, 33 are exactly 100%; getBasename is 93.52941% and getStringLengthWithMessageTag is 99.393936%. All 15 MessageUtil methods are 100%. All 959 instructions in these 50 retail methods were independently checked against the actual DOL; its identity is in `message-string-retail-proof.json`.

All edited native sources/test translation units compile with the current LLVM flags. Exact commands and statuses are in `message-string-native-commands.json`; Wii commands are in `message-string-wii-commands.json`. Compiled objects remain local evidence and must not be committed.

The existing `smg-pc-original-layout-group-tests` now exercises encoded-word string lengths across page tags and embedded terminators, picture-tag writes read through the shared parser, retained 16-bit narrowing with wider native storage, byte conversion stopping at a nonzero high byte, and basename pointer identity. The earlier page traversal cases remain. `smg-pc-message-real-or-absent-tests` now requires the original owner at direct-message entry points. Parent owns integrated runs and the real MessageHolder tests, including the retained UTF-16 clients.

## Removed duplicate Power Star demo state

The audit found `StageSessionState::_power_star_get_demo_active` had no production writer. Only its focused test toggled it, so the compatibility query could not reflect original GameScene transitions. Removed the field/getter/setter and restored the exact original `MR::isPowerStarGetDemoActive` body in the existing OriginalDemoUtil provider: `GameSceneFunction::isExecStageClearDemo()`. The native binding dispatches that to `GameScene::isExecStageClearDemo`, which checks the actual PowerStarGet and GrandStarGet nerves.

`smg-pc-stage-initialization-resource-tests` now verifies that the actual StageInitializationService-owned StageSession and SceneObjHolder cannot answer this original GameScene query. The obsolete flag toggles were removed from RestartStageSessionTests. Positive live GameScene transition validation remains blocked by the full original construction/destruction graph, including missing native destroySceneMessage/onStarPointerSceneOut providers. There was no existing positive GameSceneBinding fixture in tests. No raw-storage object, substitute GameScene, nerve flag or teardown bypass was introduced to fake one. Parent authorized the missing-owner test and explicit positive-owner frontier instead of broad constructor closure in this checkpoint.


### Integrated restart fixture diagnosis

The legacy restart-stage-session target later aborts in its synthetic player section. LLDB proves that the initially added missing-GameScene assertion had already passed; the first original MR::isPlayerDead call at line163 is intentionally caught, and the next call at line170 throws uncaught. Both call MarioAccess::getPlayerActor and require SceneObj20 (decimal0x14, MarioHolder). The test's callback attached to a generic LiveActor cannot satisfy that original owner contract. Stage-BGM/metadata assertions before this point had already completed. See `restart-missing-mario-owner.lldb.log` for both actual stacks.

The changed missing-GameScene assertion now lives in the existing StageInitializationResourceTests after real session and scene-holder binding, so it can be validated independently of the old synthetic player assumptions. Both changed test TUs compile. The broader restart/player fixture was preserved; no constant result, synthetic Mario binding or relaxed production check was added. The integrated scene-resource target now passes as recorded below.

### Independent native DAT1 review

Read-only review of the concurrently changed NativeBmgResource/OriginalMessageHolderTests found no additional issue. Parsed BMG records validate DAT1 offsets; the retained UTF16 vector uses native offsets divided by sizeof(wchar_t), preserving aliases, interior pointers and original units following terminators. Native block bytes and UTF16 words remain stable after construction. Constructor-local parsed maps/strings are no longer needed once offsets and typed records are materialized. The fixture compares eleven actual raw DAT1 code units with before/after sentinels for every fellow name and covers synthetic aliases, interior offsets, embedded tag zeros, surrogates and invalid indices.


### Passing actual scene-resource fixture

The target's resource path now initializes the actual original owners required before its camera graph: DemoDirector (matching SceneFunction::initForLiveActor) and the IgnorePauseNameObj group (matching GameScene::init). LLDB first identified CameraCover joining the missing group; after that owner was supplied, the camera graph reached an absent DemoDirector while registering an actual actor simple cast. Both missing owners are created through the real SceneObjHolder factory in their original order. No production behavior or fallback changed.

With those genuine owners, `xmake build -j8 smg-pc-stage-initialization-resource-tests` succeeds and the real-disc binary exits zero. It completes all three resource-initialization/retirement generations, including the StageSession/SceneObjHolder assertion that a real GameScene nerve owner is still required. Heap expiration, archive removal, object registration counts and scheduler retirement checks all pass. Exact build/run logs and `stage-resource-fixture-result.json` retain this validation. The exclusive Xmake lane was returned to the parent afterward.
