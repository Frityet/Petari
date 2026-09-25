# Round 12 message compatibility removal

Deleted `InformationMessageCompat.cpp/.hpp` and `MessageUtilCompat.cpp/.hpp` (four whole files).

The information-message binding had no callers. The actual `GameSceneLayoutHolder` already constructs and initializes the original InformationMessage, so its alternate singleton/registration-capture implementation is removed without replacement.

Native UTF-16 lookup now belongs to `MessageData::getMessageDirectUtf16`, beside its existing native BMG storage. `MR::getGameMessageDirectUtf16` and `MR::getSystemMessageDirectUtf16` are exposed through the canonical Game MessageUtil and select the actual process holder. The retained NativeBmgResource still supplies exact UTF-16 code units and DAT1 aliases; there is no wchar_t reinterpretation, conversion, copy or changed lifetime. Null IDs, missing holders and missing/out-of-range message indices preserve their existing results. Native extensions are guarded by TARGET_PC.

FileSelectFunc and SaveDataBannerCreator use their existing canonical MessageUtil include. OriginalMessageHolderTests retains its authored UTF-16, fellow icon and pointer-provenance assertions; only the redundant compat alias is replaced with its existing runtime::message_id_for_pointer implementation.

All 11 edited/deleted paths were initially clean, recorded in owned-manifest.json. Exact current pre-edit snapshots and scoped.patch are included. No unrelated source changes, build-file edits, builds, test runs, new fixtures or Git mutations were made. Existing canonical owners are already in the Game source graph; regenerate the compat glob after the four deletions.
