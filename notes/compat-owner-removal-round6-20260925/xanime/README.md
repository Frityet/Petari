# XanimePlayer focused regression fixture repair

The existing built `smg-pc-original-xanime-player-tests` abort was reproduced in LLDB. `abort-backtrace.log` records `SIGABRT` from macOS malloc's invalid free diagnostic at `HashSortTable::~HashSortTable`, `src/Game/Util/HashUtil.cpp:28`, through `Groups::~Groups` during destruction of the first `construction_and_shared_storage` fixture. The stack contains no new matrix utility call.

The fixture retained four independent `unique_ptr` owners of `HashSortTable::mHashCodes`, `_8`, `_C`, and `_10`. They were destroyed before the fixture's `unique_ptr<HashSortTable>`, whose actual owning destructor then attempted to free the same buffers. Native HashSortTable ownership was introduced in the earlier round2 canonical groups consolidation, documented in `notes/compat-owner-removal-round2-20260925/groups/README.md`. This secondary regression target had not been adapted.

`tests/OriginalXanimePlayerTests.cpp` now owns only the table. Removed the four array owners and their resets. The actual table destructor remains responsible for all buffers. No production source, matrix method, model setup, or assertion changed. All six test functions and main are byte-identical to the snapshot. The original empty J3DModel/J3DMtxBuffer/XanimeCore destructors were inspected; this repair does not remove the separate fixture cleanup those objects still require.

No native process fixture is needed for these bounded joint-table animation checks: the fixture deliberately uses no file or archive lookups through its null ResourceHolder. There is no replacement runtime service or scene fallback.

Before snapshot and manifest are included. `source-changes.patch` contains only the test ownership fix. No build wiring change is required. Root owns rebuilding and rerunning the test; reviewer ran no build and makes no assertion that the remaining five groups have passed yet.
