# Native exception reporting for original startup

`GameSystemException::init()` was the next missing original startup entry point.
The complete reference already exists at
`decomp/src/Game/System/GameSystemException.cpp`. Its exception UI depends on a
PPC exception thread, the Wii CPU register/map-file viewer, display console and
controller unlock sequence. The native port has no such processor context.

The native file retains the complete reference byte-for-byte in its non-PC
branch. Its explicit `TARGET_PC` branch installs Aurora's process-wide native
C++ terminate reporter. This is an architecture substitution for the exception
subsystem, rather than an emulation of `JUTException`. It deliberately does not
manufacture a console, map-file buffer or original exception-thread owner. The
actual display owner independently constructs `JUTDirectPrint` when needed.
Calling the old PPC callback on a native host reports its supplied error fields
through the real `OSPanic` boundary and aborts; no register structure is read or
fabricated.

The general Aurora reporter lives in the existing `OSReport.cpp` translation
unit. It reports the current standard exception's message (or identifies an
unknown exception / explicit terminate), then emits the real host call stack
and aborts. `OSPanic` and `OSFatal` now use the same stack reporting. macOS/Linux
use the native `execinfo` unwinder and fd-based symbol printing; Windows uses
native stack capture and prints addresses. Other hosts report that stack
unwinding is unavailable. No fake frames are emitted.

Installation deliberately replaces the process's C++ terminate handler. It is
idempotent, owns no client resources, and remains installed for the native
process lifetime. Fatal reports select host allocation routing and use a fixed
64-frame stack buffer. The native unwinder can require host runtime memory; this
is not a claim of allocation-free or async-signal-safe reporting. Native signal
handlers are unchanged: hardware faults retain the operating system/debugger's
existing handling. This does not emulate PPC hardware faults or recover from a
crash.

## Evidence

- `reference-proof.json` records byte identity of the full reference branch;
  there are no decomp changes.
- `native-syntax.json` passes all three native translation units.
- `isolated-build.json` compiles and links the actual native Game entry point,
  Aurora OS reporting implementation and fixture with no renderer, guest heap
  or device initialization.
- `isolated-run.json` records nine fresh subprocess cases. Installation returns
  zero and preserves allocation routing after repeated calls. Standard and
  nonstandard exceptions, explicit terminate, a noexcept violation, an
  unhandled worker exception, actual `OSPanic`, actual `OSFatal`, and the
  rejected PPC callback each emit their expected text plus an actual native
  stack, then terminate with `SIGABRT`. Full outputs are the per-mode logs.
- `source-manifest.json` freezes the four owned source paths.

Shared original startup execution remains the parent's next validation step.
These checks establish exception-boundary behavior, not successful complete
GameSystem initialization or Gateway gameplay. No shared build, renderer run,
display source, build-list or unrelated owner was changed by this subtask.
