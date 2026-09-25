# WPad owner consolidation, 2026-09-25

Five of the seven requested compat files are deleted. The two WPadOwnership files remain pending removal of the larger alternate pointer bootstrap; no replacement bootstrap or renamed sidecar was introduced.

## Implemented

- `WPad`, `WPadPointer`, `WPadReadDataInfo`, and `WPadHolder` now destroy their own child objects and buffers. Partial WPad/pointer/holder construction unwinds its completed children. The foreign `WPadDestruction.cpp` helper is deleted.
- Original WPadHolder methods now live in `Game/System/WPadHolder.cpp`. The actual holder owns its Aurora callback client scope and resets it **before** deleting pending WPAD info destinations. Nested client restoration remains supported by the existing SDK scope. The process already clears its client before broader retirement; holder teardown preserves the same ordering.
- WPadRumble's two callback slots use process storage, matching the original two-pad contract without borrowing the first holder's heap. Its destructor only unpublishes its own registration. WPadOwnership no longer constructs a dummy WPad to prewarm global allocation and no longer bypasses private access through explicit-template-instantiation tricks. It restores prior rumble registrations through the existing public `registInstance` method.
- Twelve speaker connection-control methods are extracted verbatim from the merged donors into `Game/Speaker/SpkSystem.cpp` and `SpkSpeakerCtrl.cpp`. Their former compat provider is deleted. The donor's empty extensionProcess is preserved. This does not import the optional speaker mixer/stream subsystem.
- RumbleCompat's sole consumer was RuntimeContext attaching an actuator to a service with no production request callers. That unused attachment and include are removed, and both adapter files deleted. Original WPadRumble continues to call WPADControlMotor directly. Legacy RumbleService and its parity event formatter remain for a later runtime cleanup.
- OriginalGameApplication now deletes the actual WPadHolder rather than manually deleting its internal allocations.

## Exact changed paths

New canonical sources:

- `src/Game/System/WPadHolder.cpp`
- `src/Game/Speaker/SpkSystem.cpp`
- `src/Game/Speaker/SpkSpeakerCtrl.cpp`

Modified existing owners:

- `src/Game/System/WPad.cpp`, `WPad.hpp`
- `src/Game/System/WPadPointer.cpp`, `WPadPointer.hpp`
- `src/Game/System/WPadHolder.hpp`
- `src/Game/System/WPadRumble.cpp`
- `src/compat/WPadOwnership.cpp`

Surgical shared-file edits:

- `src/app/OriginalGameApplication.cpp`: remove foreign helper forward declaration and replace manual WPad destruction with holder deletion.
- `src/runtime/RuntimeContext.cpp`: remove RumbleCompat include and unused actuator attachment only.

The exact pre-edit contents of all touched existing files are under `before/`, with SHA-256 values in `before-manifest.json`. The two shared-file changes have separate `*-owned.patch` files for commit-only extraction, preserving their unrelated pre-existing changes.

Deleted:

- `src/compat/WPadDestruction.cpp`
- `src/compat/OriginalWPadHolder.cpp`
- `src/compat/OriginalWPadSpeaker.cpp`
- `src/compat/RumbleCompat.cpp`
- `src/compat/RumbleCompat.hpp`

## Remaining dependency and validation

`WPadOwnership.{cpp,hpp}` still serves StarPointerDepthOwnership and six test files. Removing that alternate bootstrap coherently also touches RuntimeContext, StageSessionState, OriginalStarPointerDirector/Depth/OwnerQueries, and GPU owner tests. The canonical holder still uses the existing require_wpad_holder fallback until that graph migrates. MR allocation callbacks still live in WPadOwnership and should return to `Game/Util/MemoryUtil.cpp` using the actual HeapMemoryWatcher owner afterward. This bounded batch does not claim those two files removed.

The Game target's `**.cpp` glob includes all three new canonical sources, so no new explicit xmake entries are needed. Regenerate the source graph after deleting the old compat paths. No xmake, test, staging or commit changes were made. `git diff --check` passed for touched tracked files. No build or runtime test was run by this agent; the parent owns verification. Existing OriginalWPadOwnershipTests already exercise nested callback restoration, pending info retirement, repeated generations and failed allocation; they now exercise the actual class destructors and callback ownership.
