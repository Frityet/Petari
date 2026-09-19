# Explicit language ownership for standalone resources — 2026-09-19

The existing standalone MessageHolder owner carries an explicit original language prefix but did not publish it for MR::getLanguage. The AreaPolygon integration probe consequently dereferenced an absent GameSystem while creating MessageData, before exercising collision.

`LanguageOwnership` now retains that explicit prefix as the actual ID from the original language table. MessageHolderOwnership owns the binding through message/archive/heap retirement. Nested scoped owners restore their predecessor; an unknown prefix throws before publication. If a real GameSystem exists, its actual GameSystemObjHolder language remains authoritative. An incomplete real process cannot borrow the standalone value. Missing both owners fails explicitly.

The Game/System/Language.cpp reference remains untouched; the build selects `compat/OriginalLanguage.cpp`. Its tables and every original helper other than the ownership boundary in `MR::getLanguage` match the canonical donor exactly. See `source-equivalence.json` for hashes and the complete narrow normalization list.

Validation: `xmake build smg-pc-language-ownership-tests` passed (`build.log`); `build/macosx/arm64/debug/smg-pc-language-ownership-tests` passed (`test.log`). The test covers all 12 original prefix/ID round trips and original region helpers, nested lifetime, failed-init rollback, absent-owner rejection, and precedence of an actual GameSystem that has not initialized its object holder. Parent's subsequent full production build also passed with this source. No complete process-language boot or standalone message initialization is claimed by this focused test.

The AreaPolygon probe now reaches a separate preexisting ownership gap: `MessageHolder::initGameData -> MR::mountArchive -> MR::mountAsyncArchive -> FileLoader::requestMountArchive`, with no original FileLoader singleton. Evidence is `../original-dynamic-collision-20260919/dynamic-debug3.log`. FileUtil and fonts were not changed; the collision actor remains unregistered pending integration proof.

Commit scope:
- `src/compat/LanguageOwnership.hpp`
- `src/compat/OriginalLanguage.cpp`
- `src/runtime/MessageHolderOwnership.cpp`
- only `remove_files("System/Language.cpp")` in `src/Game/xmake.lua`
- `tests/LanguageOwnershipTests.cpp`
- only `smg-pc-language-ownership-tests` target in `tests/xmake.lua`
- this note directory
