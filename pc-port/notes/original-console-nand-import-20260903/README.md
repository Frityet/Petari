# Console NAND import and title reload isolation

Regular integration checkpoint: `smg-pc-console-nand-import-tests` now builds in
the normal macOS ARM64 LLVM 23 configuration and passes all four groups. See
`regular-build.log` and `regular-test.log`. The importer and title-only reload
are selected by the regular native build; RuntimeContext startup wiring is a
separate follow-up checkpoint.

This native resource boundary imports a selected console NAND directory into the actual owned Aurora `NandFileSystem`. It preserves the existing title save mapping and never creates substitute SYSCONF/product files. Production activation is parent-owned; the patch deliberately does not edit RuntimeContext.

## Interface and integration

`runtime/ConsoleNandImport.hpp` exposes `import_console_nand_directory(nand, directory, NandImportExisting::{Preserve,Replace})`, returning imported/preserved file and byte counts. The policy is mandatory. The intended startup order is existing title-save loading, optional `SMGPC_NAND_DIR` import with `Preserve`, then construction of the actual `SystemConfigService` before camera/configuration queries. `SMGPC_SAVE_DIR` continues to mean the current title's save directory, not the console root. Importing again with explicit `Replace` refreshes existing NAND bytes; it retains already-known permission/attribute metadata.

The importer is a runtime helper so its whole execution, including filesystem metadata, copied map, traces, buffers, and publication, is under `JkrHostAllocationScope`. This prevents persistent native storage from being allocated on a currently selected temporary original Game heap. No Game code changes are present.

Absolute paths are formed relative to the selected root and normalized by the existing actual NAND API. Both `shared2/sys/SYSCONF` and `title/00000001/00000002/data/setting.txt` therefore reach their original absolute console identities. Same basenames in different directories remain distinct. Empty files and opaque binary data are copied exactly. A plain host dump carries no NAND permissions: new identities use this backend's existing write defaults (0x3c, 0); existing metadata is preserved. No permission metadata is invented from host mode bits.

The complete scan/read/candidate-map mutation succeeds before a nothrow owned-map move publishes the import. Symlinks, non-regular entries and colliding NAND-normalized host paths reject the operation; destination files and traces remain unchanged on failure. This is an import of one ordinary directory tree, with no asynchronous persistence, SCFlush, export, or C NAND API claim. `SaveDataService`'s separate title persistence cache is still its own owner; importing console bytes does not pretend to update/export that cache.

## Title reload correctness

`Application` can select its default title save directory after RuntimeContext construction. Previously `SaveDataService::load_host_files` called `_nand.clear()`, deleting shared console resources and other titles. Its only production change in this patch is replacing that call with `erase_subtree(NandFileSystemService::title_data_root())`.

The new general Aurora `erase_subtree(path)` normalizes its input and removes the exact identity plus descendants only. Matching uses the slash boundary, so `data-sibling` and `data2` survive a removal of `data`. It records deletion of each removed file, returns the count, and leaves full `clear()` behavior unchanged. The title loader still replaces current-title contents from its selected host folder and keeps the existing relative mapping.

## Validation

`python3 build/original-console-nand-import-20260903/verify-native.py` independently compiled the importer, fixture, staged actual Aurora NAND implementation, and the entire staged RuntimeServices.cpp, then linked them before the existing frozen native archives. All four groups pass in `test.log`; no shared xmake build or GPU execution was used.

1. Import under a real 4096-byte JKR allocation domain, retire it, then read exact console/title bytes. The actual `SystemConfigService` and original SDK accessors consume imported SYSCONF and encrypted product bytes: aspect 1, Korean language 9, product area 6 and game region 4. Preserve retains the current title save; explicit Replace changes its bytes while keeping existing metadata.
2. Symlinks, normalized path collisions and a non-directory root reject without partial destination mutation.
3. Actual `SaveDataService::set_host_directory` replaces its own title data while retaining SYSCONF, RFL, another title and prefix siblings. Direct subtree checks cover normalized paths, an exact root file, descendants, absent roots, full-root erase and the unchanged explicit clear API.
4. Empty roots do not synthesize files; empty files and unrecognized binary payloads remain exact.

`compile.json` and `link-command.json` retain exact isolated commands. `manifest.json` records baseline and staged hashes. `native.patch` applies at repository root, and `aurora.patch` applies in `pc-port/aurora`; both passed apply checks. The parent should wire `ConsoleNandImportTests.cpp` as a regular fixture target with the existing Game/Aurora dependencies and integrate the environment option in its RuntimeContext bootstrap change.
