# Original J3D animation resource ownership — 2026-09-03

`J3dAnimationResource` owns a bounded copy of a J3D1 source and the actual J3D
animation objects loaded from it. The original `J3DAnmLoaderDataBase::load`
signature returns borrowed pointers retained by this owner. No partial Game
ResourceHolder, fake animation player, or new animation interpolation is used.

All twelve original file families are supported: BCK/BCA transform, BPK/BPA
material color, BTK texture SRT, BRK TEV register color, BTP texture pattern,
BVA visibility, BLK/BLA cluster weights, and BXK/BXA vertex color. Existing BCK
and BCA callers remain unchanged. The new decoder calls that same transform
resource decoder for each transform block, so its key validation, descriptor
extents, signed shifts, and ignored Full-count metadata remain authoritative.

## Original source and retail evidence

The twelve missing `readAnm*` wrappers were recovered first in the root loader
header. They simply pass the loader's actual `mAnm` to its corresponding typed
setter. The configured GC3.0a3 compiler produces **100% matching complete Key
and Full load loops and both setResource loops**. The verifier also relocates
and compares every byte of these four functions with the known RMGK01 DOL.

The missing `J3DAnmVisibilityFull::getVisibility` was recovered first in root
J3DAnimation.cpp at **0x804367D4, 0x90 bytes**. It also compares **100% and byte
identically after relocation**. It adds the verified literal 0.5 at
0x806C1B4C, converts to integer, tests that integer for negativity, and clamps
the index at the channel end. BTP instead truncates its frame directly. The
native provider uses the existing defined PPC integer-conversion helper at
these architecture boundaries.

Existing cluster and vertex-color samplers needed by the twelve-way dispatch
are imported from root. Cluster Full/Key, vertex-color constructors, and the
Full vertex-color sampler compare 100%. The Key vertex-color sampler compares
88.92473%; its recorded differences are temporary float storage/rounding and
the OSf32tou8 call/return shape. Its comparison branches and source algorithm
are unchanged. It uses the same portable signed-16 Hermite and OS conversion
surface as the already imported material families.

The original outer constructor dispatch and twelve setters were already
present in root and are preserved. Their raw objdiff percentages are explicitly
recorded, including low values: the configured compiler outlines many
JSUConvertOffsetToPtr calls that retail inlines, and constructor inlining differs.
They are not claimed as byte matches. The native source verifier reconstructs
the full import from root text and permits only these changes:

- The original constructor-dispatch body has a private native helper name. The
  public unsized entry resolves registered bounded input and retains its result.
- The original four block loops obtain first/next block pointers from the native
  owner's ordered block list. Their switch cases and setter calls are unchanged.
- Vertex-color index pointer additions use uintptr_t instead of truncating
  pointers through s32. The original element-offset multiplier remains two.
- The added sampler import uses the existing PPC conversion helper where an
  out-of-range native float-to-int cast would be undefined.

DataBase::setResource has a declaration but no supplied retail symbol/body and
is not called by ResourceHolder. No guessed implementation is added. The actual
Key/Full setResource bodies remain available; they require the decoded native
block scope, as do their load methods.

## Typed native data and lifetime

Pointer-bearing data headers widen on LP64, so raw Wii offsets cannot be cast
into those headers. Each decoded block uses the shared J3dNativeBlock builder:
actual native header/descriptor records, aligned scalar arrays, copied name
resources, and mutable material-ID and vertex-index arrays. The original
setters perform final pointer installation and vertex-index rebasing. Every
load owns a distinct native block set, avoiding a second destructive rebase of
one vertex-index array and allowing independently mutable material IDs.

Raw metadata fields used by the original loader are retained, including unknown
flag bytes, counts, matrix mode, destination register IDs, material names, and
BTK post-animation data. Unsupported BTK matrix modes are normalized by the
original setter. Unused BTK descriptor channels are retained without adding
validation for samples the actual sampler never reads. Full sample extents
come from their actual descriptors; a channel that reads a sample cannot have
zero samples. Key times permit equal ordered keys and every nonzero tangent
type uses the original four-value layout.

The file and every referenced table/value/name extent are validated before the
original loader runs. Known blocks of the wrong class are rejected before their
original unchecked cast. Unknown blocks remain in the original order and are
skipped by the original switch; repeated matching blocks retain the original
last-block-wins behavior. Unknown file types return null as in retail.

Only the owner's copied `data()` identity is registered automatically. A real
archive pointer must be registered explicitly with `register_source(span)`.
This validates the complete extent and byte identity and returns a movable RAII
registration that retains the native resource. Nested registrations reference
count one identity; final removal checks its generation. An expired or unknown
non-null identity fails before an unsized memory read. The archive's address
must remain stable for that explicit registration lifetime.

The native owner now integrates the actual retained JKR allocation-domain
service. It captures the selected original heap's registered domain, keeps
parser storage, registry nodes, control blocks and decoded tables in a host
allocation escape, then re-enters the actual domain around the unchanged
original loader dispatch. Each loaded object retains that domain. Destruction
runs the real animation destructor and releases its native tables before the
domain can dispose its original heap. Registry locks are released before heap
teardown, avoiding an inverse lock order with Game current-heap scopes.
Standalone loads outside a Game allocation scope remain host-owned. This does
not claim the real ResourceHolder constructor or Mario jump path is activated.

## Verification

`OriginalJ3DAnimationResourceTests.cpp` passes **14/14 groups normally, 14/14
with AddressSanitizer plus UndefinedBehaviorSanitizer, and 14/14 with
ThreadSanitizer**, using the actual loader, sampler and JKR runtime providers:

- All twelve concrete original animation families and BCA interpolation flag.
- Constant/full sampling, independent numeric expectations, and nonzero tangent
  type Hermite interpolation; the BCK/BCA outputs also agree with their existing
  authoritative decoder.
- BTK pre/post names, centers, matrix IDs, values, and matrix-mode semantics.
- Signed BRK values, register IDs, writable material IDs, and retained names.
- BTP truncation versus BVA half-frame rounding and endpoint behavior.
- Both vertex-color channels and native-width element-offset pointer rebasing.
- Block order, unknown blocks, source destruction, repeated loads, scoped alias
  lifetime, concurrent original load scopes, and malformed descriptor bounds.
- The actual animation allocation belongs to the selected original heap while
  raw/decoded tables remain host owned. External runtime/domain references can
  leave before sampling; final resource release then disposes the retained
  original heap only after its animation destructor.

The supplied RMGK01 `ObjectData/MarioAnime.arc` additionally passes under
AddressSanitizer plus UndefinedBehaviorSanitizer: **531/531 animation files**
(456 BCK, 4 BTK, 71 BTP) through original dispatch and actual archive aliases.
BCKs sample original transforms at start, midpoint, and end. The separate
Mario.arc contains no J3D1 animation files and supplies no animation coverage.

Run the isolated original/native verification:

```sh
python3 pc-port/notes/original-resource-holder-20260903/verify-archive.py
python3 pc-port/notes/original-j3d-animation-loader-20260903/verify.py
```

The second script includes the real archive when it exists at
`build/original-resource-holder-20260903/MarioAnime.arc`. The small accompanying
`extract-file.c` uses the installed encounter-nod C API to extract only that
resource from the supplied disc; invoke its built executable with the RVZ,
`ObjectData/MarioAnime.arc`, and that output path. Source, archive, and binary
hashes and actual compiler percentages are recorded in evidence.json. Game
assets and compiled binaries remain in ignored build directories.
