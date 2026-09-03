# Original collision lifecycle and typed resources — 2026-09-03

This checkpoint recovers the first keeper/zone lifecycle group in the root
decompilation and supplies an independent native resource owner for the actual
`KCollisionServer`. It does not attach CollisionParts to placed actors or change
the active scene collision query path. Full zone catalog and camera-code
ownership, followed by original query recovery, remain the next stages described
in `../original-collision-owner-audit-20260903/README.md`.

## Root recovery and retail evidence

| Routine | Retail address | Bytes | Original compiler comparison |
| --- | --- | ---: | ---: |
| CollisionCategorizedKeeper constructor | `0x80173B6C` | 152 | 100% |
| CollisionCategorizedKeeper::movement | `0x80173C04` | 224 | 98.21429% |
| CollisionCategorizedKeeper::getStrikeInfo | `0x80174B44` | 16 | 100% |
| CollisionCategorizedKeeper::getZone | `0x80174B54` | 160 | 100% |
| CollisionZone::calcMinMaxAndRadiusIfMoveOuter | `0x80174E54` | 236 | 100% |
| CollisionCategorizedKeeper destructor | `0x80175090` | 88 | 100% |
| KCollisionServer::setData | `0x8018318C` | 124 | 100% |
| KCollisionServer::isBinaryInitialized | `0x80183390` | 12 | 100% |

The seven 100% routines also match every actual retail byte after verifying and
resolving their relocations. All 56 movement instructions correspond after one
explicit register permutation: retail r26/r27/r28/r29 become compiled
r27/r28/r29/r26. Branch targets, fields, calls, arithmetic, and other registers
are unchanged. No conditional or instruction is omitted from that comparison.

The constructor uses the verified retail name
`地形コリジョンカテゴリキーパー`, initializes the original fields, and allocates
32 HitInfo entries. It leaves the zone array uninitialized until `getZone`
allocates one zone per `MR::getZoneNum` entry. The destructor has no array/zone
cleanup in retail; native ownership must account for the scene-heap allocation
lifetime when the full keeper is activated.

Movement visits each zone's original part count, ignores invalid members,
updates matrices only for its own category, calculates initial bounds, then
recalculates bounds when a moved part leaves the existing radius-inset box.
The `_9C` category declaration is corrected from u32 to s32 in the root and
native headers, matching the retail signed comparison. This does not alter Wii
layout. The inset-box test calls the actual original `MR::isInRange` in XYZ
order with short circuiting.

The preexisting root `setData` condition was reversed. Retail skips relocation
when `isBinaryInitialized` returns true; the source now uses `!isBinaryInitialized`.
The predicate declaration preserves the original non-inlined call. Without that
annotation the current compiler folds the predicate into a smaller equivalent
body; with it both methods match retail exactly.

`verify-source.py` runs the configured GC3.0a3 compiler with the Shift-JIS
wrapper, splits the verified supplied DOL with dtk, runs objdiff, and verifies
actual relocations and complete instruction correspondence. It also checks 15
native KCollision method bodies against the root source and the complete native
KCollision header against the root header. `source-evidence.json` retains the
results, resolved references, canonical instructions, and hashes. The DOL SHA-1
is `25c5959534b3c21246c6c7e42021b916b41fb578`.

## Native resource ownership

`resource/KCollisionResource` owns the original bytes plus aligned, typed native
storage. Resource copies share that storage. Its actual `KCLFile` contains
native-width pointers, decoded vectors, prism fields, and untouched numeric
metadata. The original four file offsets remain separately available through
`source_offsets`; they are not interpreted as host pointers.

The prism allocation includes the dummy prism at slot zero. Real prisms and the
octree remain adjacent in one allocation, preserving the original
`getTriangleNum` pointer-distance calculation and `getPrismData(i) ==
mPrisms + 1 + i`. The header's dummy-prism offset can overlap the last normals,
as the original format requires. That source overlap is decoded into separate
typed arrays. Heights stay mutable for the later original farthest-vertex
initializer; changing native prism storage does not modify archived bytes.

The octree retains its relative byte offsets. Reachable 32-bit node words and
16-bit leaf indices are decoded according to their distinct types. Leaf lists
retain their order, duplicates, zero terminator, and shared storage. Original
`checkPoint` at `0x801835D4` first advances its returned leaf pointer by two bytes,
so the preceding halfword is an unused prefix. The decoder supports that prefix
overlapping a node word or another leaf. Live indices cannot overlap live node
words in this flat little-endian representation: such a resource is explicitly
rejected, rather than interpreted with an incorrect byte order. This is a
representation limitation; it is not a claim that every such Wii encoding is
invalid. Supporting that case would need a different native octree representation
or endian-aware query loads before query activation.

Resource validation checks record extents/alignment, finite stored floats,
vector references, header shifts, reachable child offsets, subdivision depth,
leaf termination and prism indices. Root and child selection account for the
coordinate mask bits; shared reachable tables are visited without duplicating
their data. Unused bytes remain copied. More original query callers must be
audited before claiming this loader covers every encoding those callers can
reach; this checkpoint exercises original `searchBlock` and its point-query
coordinate layout.

The special one-root layout stores both axis shifts as -1. The root source now
has a TARGET_PC architecture guard that skips evaluating the original discarded
negative-shift intermediate. The original-compiler branch remains unchanged;
the native method body is copied from that root source, including the guard.

Native `setData` and `isBinaryInitialized` resolve a registry of real retained
typed KCL headers. They never inspect a truncated host pointer or reinterpret
raw big-endian bytes as the wider native header. Unknown raw attachment fails
before changing the existing server attachment.

`OwnedKCollisionServer` contains an actual `KCollisionServer`, invokes its
original constructor and initializer, retains its KCL/PA resources, and owns
the original constructor's allocated `JMapInfo`. It provides lifetime management
around the original object; no CollisionParts, collision director, or artificial
scene object is created. The server continues to borrow its file exactly as its
source methods do, and the wrapper keeps that resource alive.

## General JMap attachment boundary

`resource/JMapResource` owns bounded BCSV bytes and their decoded JMap table.
Its `data()` handle can be passed to original unsized `JMapInfo::attach` calls.
Repeated attaches share the same table/data identity. Attached JMapInfo objects
retain decoded data after the resource handle is released.

Native `JMapInfo::attach` previously returned false for every pointer. It now
resolves these retained resources, preserves the existing name, and updates the
table/data reference. Null input returns false without changing the attachment,
matching root behavior. An unregistered pointer fails explicitly because an
unsized native API cannot infer its valid extent. PA is optional: absent PA stays
absent, and out-of-range prism attribute indices remain invalid original
JMapInfoIter values. This registry is independent of the current ResourceHolder
archive wrapper; the later real holder migration can retain these resource
objects and expose their valid handles.

## Verification and remaining work

The independent fixture passes **10/10 groups** normally and under Clang
AddressSanitizer plus UndefinedBehaviorSanitizer, with both sanitizers configured
to stop on the first error. No sanitizer diagnostics were emitted. These are
real KCollisionServer constructor/init/accessor/search calls with the real BCSV
parser and JMapInfo implementation; there are no mocked collision or camera
services in the fixture.

Coverage includes independently calculated positive/negative-height triangle
vertices, local prism/attribute identity, raw-header separation, immutable source
bytes, shared mutable native storage, destruction of input buffers and external
resource handles, shared JMap identity, normal and special root layouts, nested
octree child axes, shared leaf lists, overlapping unused prefixes, zero prisms,
truncated and invalid references, and the explicit incompatible-alias boundary.

Reproduce these independent checks:

```sh
python3 pc-port/notes/original-collision-owner-20260903/verify-source.py
python3 pc-port/notes/original-collision-owner-20260903/verify-native.py
```

`verify-native.py` compiles only the six listed test/provider/parser translation
units directly; it does not run xmake or GPU work. Logs and `native-evidence.json`
contain the source and exact binary hashes. The parent integrates the fixture as
`smg-pc-original-j3d-kcollision-resource-tests` and owns shared build validation.

Native keeper source remains the existing helper subset. The newly recovered
full lifecycle is not activated yet. Importing actual CollisionParts initialization
still requires the complete ScenarioData ZoneList count, meaningful camera-code
collection/chunk ownership, original scale-mode verification, and scene/actor
allocation teardown. Original keeper → parts → KCollision queries remain a
separate source-recovery group. The active static surface registry and its
production responses are unchanged by this checkpoint.
