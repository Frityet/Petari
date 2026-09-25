# Round15 NAND owner restoration

Baseline root: `878278a2ffd451dbffc0957b7c604ca49521dfb5`. Aurora baseline and before hashes are in `owned-manifest.json`. Eighteen paths were snapshotted before editing. RuntimeContext.cpp/.hpp and AuroraNativeTests.cpp were initially dirty; exact patches preserve their preexisting changes. RuntimeContext after snapshots exclude the root's subsequent renderer cleanup.

## Ownership and implementation

Deleted `compat/NandSdkCompat.cpp` and `compat/NandSdkBinding.hpp`. The actual `aurora::NandFileSystem` now owns its SDK descriptor table and explicit activation. There is no alternate binding class or Game-dependent Aurora include. The active pointer merely selects the current filesystem; descriptors and storage callbacks belong to that filesystem. Descriptor IDs advance across activations, so a stale NANDFileInfo cannot identify a new owner's file.

The existing Aurora `lib/nand.cpp` supplies NANDInit, Create, Open, Read, Write, GetLength, Close, Delete, Move, Check, GetHomeDir, and InitBanner. It preserves access-mode checks, metadata permissions, dirty write buffering, close-time publication, invalidation before a potentially failing close commit, normalized paths, quota answer bits, and the original 0xf0a0-byte banner layout. No new async/IOS behavior is claimed. Normalized path bounds are checked consistently for create/delete/move as well as open. Raw filesystem rename now replaces an existing destination, matching the existing SaveDataService/SDK path.

A complete optional `NandIoCallbacks` table permits application serialization and persistence while the default provider operates on raw filesystem bytes. SaveDataService supplies these callbacks using its existing read/write/create/move/delete operations. GameData.bin checksum/container/native-to-retail conversion and banner scalar/UTF-16 byte conversion are preserved; GX image bytes remain untouched. Close passes the original permission/attribute through the commit callback, eliminating the old extra rewrite merely to restore metadata.

Aurora's title data root is now an instance property. Its default is the generic root `/`; the current Game storage explicitly selects `/title/00010000/524d474b/data`. No hardcoded game ID remains in Aurora's NAND provider. The SDK header is now `aurora/include/revolution/nand.h`; the root duplicate was deleted, and Aurora's umbrella header imports the canonical SDK constants/types instead of declaring duplicate constants. The original NAND declarations remain available; unrelated IOS headers are not pulled into Aurora.

`NANDManager::~NANDManager` now lives in Game/System/NANDManager.cpp. It deletes the original worker first; OSThreadWrapper's Aurora cancellation waits for completion before freeing queue/stack. Then it discards only that worker's unfinished descriptors. SaveDataService deactivates its own filesystem before destruction, dropping remaining unclosed buffered writes without publishing them. OriginalGameApplication already destroys NANDManager before scenes/resources/storage, and that order is retained.

## Caller and fixture migration

OriginalGameApplication and RuntimeContext activate their actual SaveDataService storage directly. Removed binding includes, members, and construction. No new process or save owner is synthesized.

Only mechanical existing-fixture changes were made: OriginalSystemConfig uses explicit storage activation/deactivation, OriginalGameSystemStartup activates its existing save owner, ConsoleNandImport uses explicit/instance title roots, and the initially dirty AuroraNativeTests changes one static title-root call to its actual instance. No new fixtures or test cases were added.

## Build recipe

No root xmake.lua, Game/xmake.lua, or tests/xmake.lua changes are needed. The existing aurora-os target already compiles lib/nand.cpp and is already a smg-pc-game dependency. Aurora xmake's header export now includes revolution/nand.h. CMake already lists the same source, so no CMake source-list changes are needed. Normal include lookup resolves the deleted port header to Aurora's canonical header. Root compatibility wildcard selection drops the deleted provider automatically.

## Validation boundary

No builds, tests, runs, staging, or commits were performed by this lane. Scoped root and Aurora `git diff --check` passed. Source/test search found no references to NandSdkBinding, NandSdkCompat, or the removed static title_data_root API. An unrestricted root diff check also reported preexisting whitespace in notes/gateway-wakeup-demo-20260912/first-run.log; that unrelated file was not edited. Root owns the integrated app build and 120-frame smoke.

Artifacts: `root-nand-only.patch`, `aurora-nand-only.patch`, `dirty-runtime-only.patch`, before/after snapshots, and the manifest. The root patch excludes all other agents' post-freeze RuntimeContext changes.

## Integration compile correction: storage transactions

The integrated build found ConsoleNandImport still copying and move-assigning the entire filesystem. Added this nineteenth owned path, captured its clean baseline and a separate transaction-fix-before snapshot for all three follow-up files. `clone_storage()` returns an inactive filesystem with copied file bytes/metadata, quota values and trace; C++ guaranteed copy elision avoids restoring copy/move support. `swap_storage()` is nonthrowing and commits only those storage fields. It never changes either owner's title root, activation, callback table or SDK descriptors. An existing descriptor continues owning its buffered contents and can only publish through its normal close path.

Importer now stages every read/allocation against that isolated clone and swaps only after all files and accounting succeed, preserving failed-import rollback including trace. Actual app imports still occur before activation/worker startup; the general storage transaction API requires serialization with other storage access. No new framework or fixtures. Source frozen for root retry4; no build/test run by this lane. Regenerated root/Aurora patches use owned after snapshots, preserving the root's unrelated later RuntimeContext edits.
