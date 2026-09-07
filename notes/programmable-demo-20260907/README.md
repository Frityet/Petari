# Original demo request holder on native pointers — 2026-09-07

## Scope

Restore the original DemoStartRequestHolder as a usable native prerequisite for
programmable demos. Its existing allocation, fixed-slot lookup, copy, FIFO,
wraparound, queue-capacity and slot-reset algorithms are retained. This is a
request-storage component; it does not claim to start a playable programmable
demo, arbitrate `MR::canStartDemo`, or provide camera/Mario ownership.

## Source and type evidence

The existing `DemoStartInfo` header represented every field except the demo name
as `u32`, including actor, layout, executor, nerve and string pointers. The four
original `find` overloads also truncated their requester pointers to `u32`.

The RMGK01 DemoStartRequestUtil assembly establishes the true pointer fields:

| Offset | Type | Evidence |
| --- | --- | --- |
| 0x00 | LiveActor* | LiveActor request at 0x800BE610; starter/nerve dispatch |
| 0x04 | LayoutActor* | LayoutActor request at 0x800BE740; starter/nerve dispatch |
| 0x08 | NerveExecutor* | executor request at 0x800BE858; nerve dispatch |
| 0x0C | NameObj* | starter selection at 0x800BEE70 |
| 0x10 | NameObj* | companion actor at 0x800BE878; movement at 0x800BEF34 |
| 0x14 | DemoExecutor* | findDemoExecutor result stored at 0x800BEE24 |
| 0x18 | const char* | existing demo-name field; common setter at 0x800BEE04 |
| 0x1C | const char* | requested part name stored at 0x800BEE28 |
| 0x20 | const Nerve* | LiveActor request at 0x800BE620; setNerve at 0x800BEEAC |

The fields now use these actual types, their constructor uses nullptr, and the
lookup functions compare typed pointers directly. Remaining scalar flags retain
their original representation. Field names and declaration order are preserved.
Explicit specialization declarations in the header make the existing ring
specializations valid before their first use on standard-conforming Clang; no
ring algorithm was changed.

`DemoStartRequestUtil::isEmpty` was absent as C++ source. Its 76-byte function at
`0x800BEF58` was recovered into `src/Game/Demo/DemoStartRequestUtil.cpp` first.
It tests only the four requester pointers (LiveActor, LayoutActor, NerveExecutor,
NameObj), in that order. Companion-owner/metadata pointers alone do not occupy a
slot. The original function matches exactly. No inline assembly is used.

## Validation

GC/3.0a3 compilation with the current Game flags passes both translation units.
The target objects come from a fresh `dtk dol split --no-update` of the original
RMGK01 DOL, verified SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`, using current
symbol/split definitions. Target objects and reports remain local parent notes.

- Complete DemoStartRequestHolder: 99.41111% text fuzzy match, 19/19 paired code
  symbols, 18/19 exact. The sole non-exact function is findEmpty at 91.969696%.
- New isEmpty helper: 100% exact, 76 bytes. The rest of DemoStartRequestUtil
  remains undecompiled; its whole 3212-byte unit consequently scores 2.366127%.
- Separate PPC compile assertions verify DemoStartInfo size 0x38, demo-name
  offset 0x18, start-nerve offset 0x20, and holder proxy offset 0xA0.
- The port receives byte-identical copies of both sources and both headers.
- LLVM 23 native build and the focused executable pass. The test checks typed
  requester lookup, original copy/reset and FIFO wraparound/capacity, all sixteen
  record slots, proxy ownership during real scene-registration capture rollback,
  and holder lifetime after a temporary Game heap retires.

## Native ownership and remaining dependencies

The parent port's `compat/DemoStartRequestOwner` scopes the original holder
constructor to host allocation, claims the constructed proxy NameObj against
independent scene-capture deletion, and deletes all sixteen original records,
the proxy, and the holder at retirement. Those lifecycle changes are outside
Game source. The stored requester/nerve/executor/string pointers remain borrowed.
Callers must preserve their lifetimes until the request is consumed or its holder
is retired. Original low-level preconditions remain: do not register more than
sixteen occupied records, and only pop a nonempty queue. A full queue retains its
original behavior of ignoring an extra push.

The actual programmable start path still needs DemoStartRequestUtil start/request
orchestration, SceneNameObjMovementController, CinemaFrame, MarioAccess remote
demo behavior, and their end/cancel coordination. Existing explicit failures
remain; this change does not substitute a DemoSheet executor or stage-specific
trigger for those missing systems.

## Next integration boundary, recovered call ordering

The original start helper at 0x800BE3A4 first requests the appropriate scene
movement stop, then enters the director (and optionally its time-keep executor),
deactivates the ordinary layout, starts the optional cinema frame and pointer
mode, restores the starter/camera/lens-flare movement, optionally deletes
one-time effects, enters Mario remote-demo readiness for movement modes 2/3,
and finally broadcasts ACTMES_START_DEMO. These are actual required owners;
disabling player input alone does not reproduce this sequence.

The LiveActor request helper at 0x800BE544 selects the success nerve before an
immediate start. If MR::canStartDemo rejects starting now, it selects the
waiting/failure nerve, stores the success nerve and start options in a free
record, queues that record and returns false. Returning false therefore does
not mean that the request was discarded. The original holder performs ordered
first-match lookup by typed owner and name, without priority sorting or
uniqueness enforcement. Its queue holds record pointers, not copied values.

Original DemoDirector::endDemo prioritizes an already pending request; otherwise
it can wait for camera interpolation with an overwrite movement mode, or end
immediately. Final teardown restores scene movement, ordinary layout and the
requester's pointer mode, releases the cinema frame when owned, then ends Mario
remote control for movement modes 2/3 before clearing active state. The new
storage component deliberately does not manufacture these missing actions.
