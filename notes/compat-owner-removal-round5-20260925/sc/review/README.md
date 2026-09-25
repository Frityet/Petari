# Independent SC migration review

Reviewed the completed root migration without editing production sources or tests and without running a build. The before-source reference is `../before`, including the initially dirty RuntimeContext snapshot. No new blocking finding remains after restoring the C bridge's outer interrupt scopes.

`python3 notes/compat-owner-removal-round5-20260925/sc/review/verify_source.py` passes all 29 checks; `source-evidence.json` records the reviewed source hashes and individual results.

## Ownership and dependencies

The SDK declarations now belong to `aurora/include/revolution/sc.h`, copied byte for byte. `aurora::SystemConfiguration` owns the decoded catalog and the borrowed OS product-memory range; it borrows the supplied `aurora::NandFileSystem`. It has no Game, compat, or smgpc dependency. The former `compat::JkrHostAllocationScope` is literally an alias for the new directly named `aurora::allocation::HostAllocationScope`, so allocation routing is unchanged.

Both OriginalGameApplication and RuntimeContext retain their prior member order, construction order, and teardown behavior. Their save/NAND owner still outlives SystemConfiguration. Comparison against the recorded pre-edit snapshots confirms only the type/include migration, avoiding confusion with RuntimeContext's earlier unrelated working changes. The host's existing initial console-language policy remains in OriginalGameApplication; it has not been moved into the generic SDK catalog.

Both `aurora/xmake.lua` and `aurora/cmake/aurora_os.cmake` add SCSystem, SCapi, and SCProductInfo once to the OS library beside nand/sysconf. Xmake publishes both headers, and CMake already publishes the include directory and links the base/Threads dependencies. There are no remaining references to the retired service/provider names under src, tests, or Aurora.

## Behavior preservation

- The complete catalog implementation matches the previous service after namespace and allocator-alias substitution. SDK ID order, first duplicate selection, unknown record preservation, typed reads, native u32 output from big-endian payload, small/big array threshold, fixed file size, 70-byte reserved tail, and dirty-state rules remain unchanged.
- All six C bridge bodies match the previous wrappers. Each takes the outer interrupt scope before looking up the published owner, retaining protection against concurrent retirement; each catalog method also retains its nested scope.
- Missing/short SYSCONF input still produces the original cleared-file path, while a malformed full file invalidates the index. Type/size replacement still deletes the prior record before a failed create, as the original SDK did. This is intentional behavior, not a new failure-atomicity guarantee.
- OS initialization remains an explicit precondition. Overlapping owners are rejected before publication or replacement, and retirement restores the exact prior 256 bytes. Missing/short product reads retain untouched boot bytes except the original terminator at byte 255.
- SCapi and SCProductInfo match their old complete bodies token for token. Defaults, field ranges, encrypted product lookup, regional mappings, and failure/output behavior therefore remain intact. The migration does not implement the SC header's previously declaration-only APIs.

## Validation boundary

The eight existing OriginalSystemConfigTests groups retain their entire test logic after the type change. They cover SDK defaults, product decoding, byte order and ranges, Bluetooth arrays, duplicate indexing, delete-before-create, malformed/short/capacity handling, explicit absent-owner behavior, overlap rejection, and OS memory restoration. Seven other consumer test files likewise preserve their logic.

This review verifies source equivalence, dependency placement, and lifetime order. It does not establish new runtime results; the parent task owns the application build and execution of the SC and integration tests.
