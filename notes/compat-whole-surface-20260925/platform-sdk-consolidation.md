# Canonical SDK owner consolidation

Fourteen compat translation units were removed and replaced by their actual canonical `src/JSystem` owners, using merged `decomp/` revision `1a126cb5d` as donor. Ten resulting files are byte-identical to their donors. Four retain necessary existing native adaptations:

- `JKernel/JKRFileFinder.cpp`: empty base destructor required by the native header/vtable. The donor already owns the remaining constructor/iterator/destructor bodies.
- `JAudio2/JAISound.cpp`: `%p` and `static_cast<void*>` for the host pointer diagnostic, plus the public `JAISoundID` conversion instead of type-punning.
- `JAudio2/JAIStream.cpp`: `uintptr_t` for ARAM address passed to the native-width JAS stream API.
- `JAudio2/JAIStreamMgr.cpp`: `uintptr_t` for ARAM address retirement through the native-width manager interface.

The exact removed/destination/donor paths, before/after SHA-256 values, and complete donor diffs are in `platform-sdk-consolidation.json`. JAIAudible and JAIAudience were added to the initial lane by the parent; both are exact donor copies. They remain part of the parent's original scope accounting rather than the 91-file platform/resources audit count.

No source changes were made outside these 14 canonical SDK destinations and their deleted compat counterparts. No Game sources, xmake files, tests, staging, commits or builds were changed/run by this agent for this batch.

The parent needs to add these explicit sources to the `smg-pc-game` target; the removed providers were previously included by the compat glob:

```lua
add_files({
    "../JSystem/JKernel/JKRDisposer.cpp",
    "../JSystem/JKernel/JKRFileFinder.cpp",
    "../JSystem/JSupport/JSUList.cpp",
    "../JSystem/JUtility/JUTNameTab.cpp",
    "../JSystem/JAudio2/JAIAudible.cpp",
    "../JSystem/JAudio2/JAIAudience.cpp",
    "../JSystem/JAudio2/JAISound.cpp",
    "../JSystem/JAudio2/JAISoundChild.cpp",
    "../JSystem/JAudio2/JAISoundHandles.cpp",
    "../JSystem/JAudio2/JAISoundStarter.cpp",
    "../JSystem/JAudio2/JAIStream.cpp",
    "../JSystem/JAudio2/JAIStreamDataMgr.cpp",
    "../JSystem/JAudio2/JAIStreamMgr.cpp",
    "../JSystem/JAudio2/JASSoundParams.cpp",
})
```

Validation here is read-only donor comparison and destination mapping. Parent build/link and focused SDK tests remain required before claiming validated native behavior.
