# Canonical JSU stream and JKR archive owners

Baseline: `3b37917cdcc04ac7e2ce190cab9f9835fe2b95fa`. Every changed existing file was clean at capture; snapshots, hashes and Git status are in `before/` and `before.json`. This agent did not run builds, edit xmake, stage, commit or push. Sources are frozen for root's combined build. `source-changes.patch`, `owned-patches/`, `build-wiring.json` and `audit-update.json` describe the exact scope.

## Ownership and source closure

Deleted `src/compat/JSUStreamCompat.cpp`, `JKRArchiveCompat.cpp`, and `OriginalArchiveIndex.cpp` in full. Canonical compiled owners are:

- `JSystem/JSupport/JSUInputStream.cpp`: sequential and random input methods plus native virtual destructor.
- `JSystem/JSupport/JSUOutputStream.cpp`: sequential and random output methods plus native virtual destructor.
- `JSystem/JSupport/JSUMemoryStream.cpp`: both memory stream owners and one local widened seek implementation, matching the donor's combined file boundary.
- `JSystem/JKernel/JKRFileLoader.cpp`: complete currently declared file-loader implementation, static volume list/current loader, and the original SDK volume mutex.
- `JSystem/JKernel/JKRArchivePub.cpp`: public lookup/read/catalog methods and duplicate-mount lookup.
- `JSystem/JKernel/JKRArchivePri.cpp`: native metadata lifetime/decoding and private lookup machinery.
- `JSystem/JKernel/JKRMemArchive.cpp`: archive construction, publication, destruction, fixed mounts and indexed fetch/decompression.

These are the actual class owners and donor file boundaries, rather than renamed mixed providers. Nine native archive method bodies previously inline in JKRArchive.hpp now live in their corresponding public/private implementation file. Public signatures and data ownership remain unchanged. No Game-specific policy or Game/compat includes were introduced; existing host-allocation scopes now use Aurora's actual HostAllocationScope directly instead of the compat alias. No Game sources changed in this batch.

The parser in `resource/RarcArchive` remains the existing generic format machinery. This consolidation preserves the current native archive API, including existing const resource lookups and path resolution. It does not claim completion of all original SDK mount modes, type-filtering/directory resource semantics, or missing DVD/ARAM archive APIs; replacing the parser or broadening those APIs would be a separate coherent SDK task. No missing-method stubs were added.

## Native semantics retained

JSU preserves raw bytes and partial-transfer counts; unread destination suffixes remain untouched. Wrapper read/write set sticky IO_ERROR on short transfers, direct readData/writeData retain their existing state policy, and seek clears IO_ERROR while returning displacement. End-relative seeking remains `length - offset`, with arithmetic widened before clamping. Invalid native spans transfer nothing; negative lengths produce empty spans. Existing host-native scalar helpers and the explicit serialized-format readBig/writeBig templates are byte-unchanged. Existing 12-case JSU tests were left byte-unchanged.

JKR preserves endian-decoded metadata in native-width typed arrays, raw authored flags/IDs/catalog order, native pointer mount identity, borrowed versus owned archive bytes, duplicate mount reference increments, and exact resource address/size identity. Mount metadata validates before publication. Owned fixed buffers must originate from an actual JKR heap, and destruction discards borrowed parsed metadata before freeing owned bytes through their original heap. OriginalArchiveIndex's two methods are retained unchanged. Original root `..` sentinel handling and bounded metadata/name validation remain intact.

The shared recursive volume lock is now the original `JKRFileLoader::sVolumeListMutex`, with a protected VolumeLock for derived archive implementations. Its initialization remains guarded by the same once-only policy: process startup cannot reset a list or mutex whose archives were already mounted. Nested mount publication and duplicate lookup use the same lock, and unmount still releases the lock before deleting the loader. The list's existing native alias and current-loader behavior are preserved.

## General bounds correction

Both existing CArcName::store overloads allowed 256 bytes into mName[256] and then wrote the terminator one byte beyond it. Metadata parsing already rejects names of length 256 or more. Requests now reject the same oversized component with a host-owned invalid_argument before the out-of-bounds write. Valid 0–255-byte names retain the original hash, lowercase, length and delimiter behavior. This is the only algorithm change in the 58-method inventory.

OriginalJkrArchiveTests adds two focused cases (eight total): the 255/256-byte boundary for both name overloads with neighboring storage guards; and borrowed fixed-mount publication, stable resource pointers, global lookup, repeated list initialization, duplicate-reference behavior and teardown. The latter mutates the mounted bytes and checks that all lookup paths retain their actual caller-owned address.

## Static verification and root validation

`python3 notes/compat-owner-removal-round4-20260925/sdk/validate-source.py` passed. It checks all 58 previous method bodies: 56 unchanged under formatting, scope-alias and mutex-name mapping; two exactly checked name-boundary guards. It also verifies unchanged explicit-endian headers and JSU test source, absent retired providers, and no Game/compat includes in the new SDK owners. See `source-validation.json` and `method-inventory.json`. Scoped `git diff --check` passed; retired provider references are absent from src/tests.

Root should add the seven source paths listed in build-wiring.json and regenerate the compat source graph. Focused targets: `smg-pc-original-jsu-stream-tests`, `smg-pc-original-jkr-archive-tests`, and `smg-pc-original-file-loader-tests`. Existing save-owner checks cover explicit endian templates. This note records static evidence only; root owns compile/runtime results.

## Auto-effect process fixture follow-up

Root reports the SDK batch passing. A later bounded test-only migration replaces OriginalAutoEffectMetadataTests' obsolete standalone particle publication with the actual original process fixture. Authored metadata assertions are unchanged; resource identity, process heap ownership and weak backing retirement now refer to the original GameSystem/FileLoader. See `auto-effect-fixture/README.md`, static validation and exact patch. Root owns runtime validation of that follow-up.
