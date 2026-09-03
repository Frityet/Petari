# Original SC accessors and runtime configuration owner

Staged source and seven CPU regression groups are ready for review. Production
RuntimeContext and Aurora sources have not been modified by this package.
The owner reads actual absolute NAND identities; it does not infer console
settings from a camera, renderer, disc region, or a stage name.

`SystemConfigService(aurora::NandFileSystem&)` must be created after the runtime
has imported its NAND files and before any original screen/camera configuration
query. It requires initialized actual OS/MEM1 and owns one SDK configuration
catalog. `reload()` rereads its backing NAND explicitly. Destruction unpublishes
the owner and restores the prior 256-byte OS product range. Overlapping owners
are rejected before any state mutation. Host metadata allocations escape Game
heap scopes, and SDK entrypoints acquire the actual OS interrupt guard before
resolving the active pointer, protecting lookup against owner retirement.

## Original code and binary proof

All nineteen `scapi.c` accessor/setter bodies are imported intact, with six raw
integer ID literals replaced by their existing enum constants to compile as
C++. The root-first patch has no Wii code effect. The complete original
`scapi_prdinfo.c` is imported byte-for-byte, including `__SCF1`, area/region
mappings, and its reads through real `OSPhysicalToCached(0x3800)`.

`verify-original.py` compiles with GC3.0a3 and resolves every relocation against
the verified RMGK01 DOL (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`).
**22 methods, 1,784 bytes match exactly**, including all nineteen accessors
and all three product functions. Both complete product tables match the DOL
(70 and 24 bytes). The encryption seed/rotation, keyword comparison, partial
output failure, area/region codes, and language fallback execute the original
code rather than a rewritten decoder.

The native SDK header retains the actual public types, IDs and declarations.
It removes the unused private SCControl aggregate and its BTE/NAND declaration
include dependencies; those full asynchronous APIs are not implemented here.
The identical u8 Bluetooth aliases avoid importing unrelated BTE internals.
No fabricated NANDFileInfo/SCControl structure is provided.

## Typed compatibility contract

The six selected typed lookup/replace functions operate on Aurora's actual
owned SysConf resource using the original 36-entry name/ID map. Byte and Bool
are distinct. U32 payloads are decoded from their original big-endian bytes.
Array lookup accepts both wire array types and requires exact requested size.
Failed lookups preserve caller output. The original integer API's valid-output
pointer precondition remains; the array API explicitly rejects nullptr.

The SDK runtime index selects the first matching record at load. An in-place
replacement preserves record type/order and only marks dirty when bytes change.
A type/size-changing replacement deletes the selected record, marks dirty,
then attempts to append its replacement. A failed create leaves the deletion
in place. Existing duplicate names are not reselected until a reload. General
indexed `replace_at`, `append`, and `erase_at` operations are therefore added to
staged Aurora SysConf, preserving its existing name-based APIs and checked
resource ownership. Unknown records and payloads survive SDK mutations.

SC's actual 70-byte tail reservation is preserved (retail `addi r3,r31,-70` at
`0x804D0A18`). Capacity is checked before reading/copying a new payload. Native
runtime indexes are separate typed storage; no host-endian pointers or offset
references enter the wire document.

The original NAND reload expects 16,384 SYSCONF bytes. Missing/short reads are
cleared before parsing, yielding the valid empty catalog whose original getters
supply their defaults. Malformed full-size data clears the document but leaves
the runtime index unavailable, as the original failed-parse path does; creates
therefore fail. A longer file supplies the first fixed-size read. Product reads
copy at most the actual 256-byte range, preserve untouched boot bytes for
missing/short input, and apply the original final-byte-zero rule. Owner
retirement restores the complete prior range, including that last byte.

## Validation

`OriginalSystemConfigTests.cpp` uses actual Aurora NAND, SysConf, OS/MEM1, SDK
accessors and product decoder. It verifies:

* Every original numeric default and unchanged failed outputs.
* Real encrypted JPN/USA/EUR/KOR/CHN/ROC product inputs, region tables, language
  fallback and short product output buffers.
* Strict Byte-vs-Bool, U32 byte order, exact arrays, all 256 signed display-offset
  inputs, unchanged-replacement dirty state and original getter clamping.
* Complete original Bluetooth device/paired-device structures and the 256/257
  small/big array boundary.
* Duplicate selected-ID behavior, delete-before-create failure, preservation of
  unknown LongLong data, and index reconstruction on reload.
* Missing, short, malformed and capacity-limited documents.
* Owner absence/overlap failure, mapped boot-memory retention/restoration,
  retirement, and subsequent reconstruction.

All seven groups pass in an isolated CPU executable. Exact compiler/link
commands, source hashes and correspondence are in `native-evidence.json`;
compact complete output is `test.log`. No shared xmake or GPU run was used.
The suggested regular game-linked test target is
`smg-pc-original-system-config-tests` using
`tests/OriginalSystemConfigTests.cpp`.

## Patches and remaining boundary

Apply `root-enum.patch` before `native.patch`. `aurora.patch` applies inside the
nested Aurora repository and only adds general indexed resource operations.
The staged files remain in `build/original-sc-service-20260903`.

No no-op SCInit, SCCheckStatus, SCFlushAsync, or NAND C API has been added.
Original async reload/flush completion, error/dirty retry and host persistence
remain a separate SDK/NAND closure. Current replacement APIs change the owned
RAM document, as original setters do; they do not silently write a settings
file. The service exposes document/dirty diagnostics to its explicit owner.

Current SaveDataService host import maps relative paths beneath the title's
NAND data root. Thus `SMGPC_SAVE_DIR/shared2/sys/SYSCONF` is not the absolute
console path consumed by SC. A separate general console-NAND root importer is
needed to populate `/shared2/sys/SYSCONF` and
`/title/00000001/00000002/data/setting.txt` from an actual user-supplied dump.
It must preserve existing SMGPC_SAVE_DIR title mapping; no default files are
synthesized here. RuntimeContext wiring is intentionally left to the parent
while its archive/bootstrap edits are active.
