# Original ResourceHolder ownership migration

The native service now constructs and returns the actual Game `ResourceHolder`.
Its complete source and header are byte-identical to the authoritative root.
The unrelated global archive-wrapper class has been removed atomically with all
of its callers. Shared builds, holder and complete animation-loader sanitizers,
and fresh Title/Gateway smokes pass. The exact-unit OnlyCamera fixture
expectations were corrected from verified retail rounding, and the newly
reached animation block-size issue was fixed at the general decoder boundary.
This checkpoint does not activate MarioAnimator/ModelManager or claim jumping,
the bunny chase, or full gameplay.

## Original behavior and retained ownership

The original constructor executes recursive JKR finder order, case-sensitive
substring dispatch, hashed names, actual typed J3D loaders, `MaterialAnmBuffer`,
`BckCtrl`, and eight effect-matrix backups per material without a Game algorithm
change. Previous original-compiler verification of all holder and resource-table
methods is in `../original-resource-holder-activation-20260903/`.

`FileLoader::requestMountArchive` selects an explicit heap or the current heap;
`ArchiveHolderArchiveEntry` retains it and `ResourceHolderManager` restores it
around construction. Multiple archives therefore share a real heap. The native
service receives one explicit retained cohort rather than assigning heap
identity from an archive or actor name.

| Owner | Retained data and release contract |
| --- | --- |
| `GameResourceRuntime` | Actual JKR root and reserved MEM1 resource heap. Construct after Aurora configuration and inject into all contexts; no per-context OS allocator reset. |
| `ResourceHolderService` | Injected DVD service, actual JKR cohort, path-keyed shared archive owners. Returns real `ResourceHolder*`; `retain(holder)` supplies an explicit lease that may outlive the service. |
| `ResourceArchiveOwner` | Shared immutable RARC, actual `JKRMemArchive`, typed models/animations/JMap aliases, and actual holder. Models die before their attached MaterialAnmBuffer; all native backing and SDK objects die before the shared cohort. |
| `J3dModelResource` | Complete original model/material table, mutable typed components, MEM1 textures and JKR domain. Each original load creates independent mutable SDK state. |
| `J3dAnimationResource` | Native metadata/value tables, original animation instances, exact archive source aliases, and each instance's allocation domain. |
| `JMapResource` | Typed BANMT, retained source alias, and host-owned cached strings for the full BckCtrl lifetime. |

`backing(holder)` exposes archive diagnostics explicitly without adding native
wrapper fields to the Game class. Foreign holder identities are rejected. The
DVD cache retains shared RARC handles while preserving existing reference APIs,
resolved keys, counts, and traces; it does not duplicate the RARC for each holder.
The cache-entry function itself enters a host allocation scope so direct callers
inside a temporary Game domain cannot leave the cache owned by that domain.

Only the backing required by original dispatch is prepared before construction.
Raw files retain actual archive byte identity. A zero-length BCK preserves the
original null table entry; original null animation/JMap dispatch does not need a
fabricated alias. No incomplete model or holder is published.

The outer construction order is `JkrAllocationScope`, reusable
`J3dCommandScope`, then load-mutex exception recovery. This covers original
post-load helpers that generate VCD/VAT commands after SDK load returns. On a
native loader exception, only extra current-thread recursion of original load
mutex 0 is released; prior caller ownership, GD, heap, interrupt and scheduler
state are preserved. Original SolidHeap allocations made before an exception
remain consumed until the shared cohort dies. Individual frees remain original
no-ops; the implementation never rewinds a heap containing other live objects.
Allocation-exhaustion recovery was not exercised by this fixture.

## Startup and consumer changes

`RuntimeContext` now requires an injected `GameResourceRuntime`. Application DI
constructs it after AuroraWindow; both showcase routes, debug probes and all
current context fixtures follow the same lifetime order. There is no legacy
constructor that silently creates an OS allocator. The resource budget is caller
policy through `GameResourceBudget`/bootstrap configuration: defaults are a
128 MiB host root, 64 MiB cohort and 16 MiB MEM1 reservation.

Collision and PlanetMap callers use the retained archive association for raw
resources and path diagnostics. Their existing runtime behavior is unchanged;
this migration neither attaches fabricated CollisionParts nor activates the
unclosed original collision lifecycle. The parent also imported the original
material/visibility players and J3DMaterialTable attachment methods, documented
in `animation-players.md`; the holder test executes actual BpkPlayer attachment,
frame reflection, RGBA calculation, detachment and stop.

Measured fixture usage: the cohort containing actual Mario plus the temporary
mixed Mario-model/BPK holder consumes 39,024 bytes; 16,674,976 of the reserved
16,777,216 MEM1 bytes remain after the temporary model is destroyed. The separate
stationed-archive fixture reports a 171,328-byte cohort and 16,612,768 available
MEM1 bytes. These are concrete small fixtures, not a full-stage budget claim.

## Production matrix ABI defect exposed by the holder

The first mixed-model fixture found backup `[material0][slot0][1][1] == 0` while
the live initial matrix contained `1.0`. The mismatch existed immediately after
construction, before any BpkPlayer calls. The exact original 4x4 backup algorithm
was correct. Both real Aurora matrix library targets omitted `TARGET_PC`, so
`dolphin/types.h` selected `unsigned long u32`: eight bytes on macOS LP64.
The linked `PSMTX44Copy` consequently copied 16-byte pairs, masked 64-bit values,
and wrote eight bytes beyond its final matrix. This was a general SDK ABI/build
error, not a model-specific matrix rule.

Both Xmake and CMake matrix targets now publish the same `AURORA`/`TARGET_PC`
definitions as other Aurora targets. This also restores native-width consistency
for matrix-stack and integer-count APIs. C compile-time width assertions reject
the incorrect library configuration. The CMake `mtx44_tests` target now links
the real `aurora::mtx` library rather than compiling a private C source with its
own definitions. Its four existing identity/bounds, exact float-bit/subnormal,
in-place and paired-overlap groups all pass. The earlier raw-retail paired-store
arithmetic proof remains unchanged; the actual library now compiles that body
with the intended types.

The holder fixture now snapshots all original effect matrices immediately after
construction, verifies every copied/identity element, then proves the retained
backup remains unchanged after real material animation and an explicit live
matrix mutation. It does not derive expected output from the implementation.

## Verification and limits

- `verify-source.py`: literal original ResourceHolder source/header equality,
  exactly one global class definition, 39 migration source hashes.
- `OriginalResourceHolderTests`: all four groups pass with the supplied RVZ,
  including recursive metadata/file IDs, BCK/BCA/BANMT, borrowed-name lifetime,
  zero-size animation, exact heap identity, typed model/BPK, immutable backup,
  loader-exception lock restoration, cache/domain destruction, and service reuse.
- `verify-native.py`: the final decoder source passes all four holder groups
  plus all fifteen animation-resource groups and 531 real MarioAnime files
  (456 BCK, four BTK, 71 BTP) under ASan/UBSan with leak detection enabled.
  Two executables share 87 instrumented providers and each links its own test
  object: 88 sources per executable, 89 compiled in total. The all-twelve-family
  navigation/truncation tests are instrumented, including deliberately unused
  final next pointers and actually missing sample data. Coverage includes holder,
  typed owners/decoders, Game tables/control/player, DVD service cache, original
  factories/finalizers/material blocks/traversal, all six Aurora matrix C files,
  JKR heap and OS lifecycle providers. Other retained libraries are frozen and
  uninstrumented; this is not whole-program instrumentation. Source/header,
  archive and build-flag hashes are checked before and after the run.
  Original SolidHeap suballocations share their backing allocation and do not
  receive individual ASan red zones; matrix bounds and owner lifetimes also have
  explicit functional assertions rather than relying on sanitizer coverage alone.
- `verify-matrix-abi.py`: verifies real Xmake/CMake definitions and library link,
  deliberate missing-definition compilation failure, and all four matrix groups.
- `verify-matrix-integration.py`: the matrix ABI sweep builds all 15 targets.
  All fourteen CPU programs pass: holder, camera vector math, Game rotation
  math, original camera, OnlyCamera,
  camera view interpolator/service, stage/event cameras, Xanime core/player,
  J3D joint traversal, fixed-step clock and complete original model resource.
  OnlyCamera now passes nine of nine cases after a test-only correction: the
  verified retail magnitude of a unit vector is the float just below one, while
  the next input float produces exactly one. No Game or math body was altered.
  The original seven-of-nine diagnostic result is recorded in the prior parent
  run, and the final matrix evidence references the independent corrected test
  log. Showcase is built but excluded from this CPU run. Per-target binary
  hashes, logs and results are retained. Following the final decoder correction,
  the parent rebuilt and passed the affected holder/animation-loader tests; the
  final sanitizer independently covers those same corrected sources.
- Parent GPU evidence records a passing two-frame Title smoke, five-frame
  Gateway smoke, and five-frame actual base-J3D draw probe with the authored Run
  animation sampled at frame 15. The first Gateway
  attempt exposed a final animation block that declares four bytes beyond its
  retained file; the original SDK does not dereference that final next pointer.
  General file-bounded reads now preserve original block navigation metadata
  without padding, tolerance or asset-specific rules. See
  `animation-block-bounds.md` and the final `*.bounds-smoke.log` files.
- The earlier isolated syntax sweep passed 31/32 files. Its existing
  CenterScreenBlur fixture references absent `isDead`/`updateNerve` APIs; only
  startup injection was changed there. It is not included in these runtime gates.

The initial DVD-cache fixture used a host-directory assumption inconsistent with
Aurora's DVD/FST service. It was corrected to read the supplied disc's actual
InvisibleWall10x10 archive; no production host-file fallback was added.

## Reproduce

```sh
python3 pc-port/notes/original-resource-holder-migration-20260903/verify-source.py
python3 pc-port/notes/original-resource-holder-migration-20260903/verify-matrix-abi.py
python3 pc-port/notes/original-resource-holder-migration-20260903/verify-matrix-integration.py
python3 pc-port/notes/original-resource-holder-migration-20260903/verify-native.py --jobs 3
```

The last two scripts use the supplied repository RVZ, or `SMGPC_REAL_DISC` for
the sanitizer. Original assets and binaries remain in ignored build/resource
storage. Run shared Xmake and isolated sanitizer work under the parent freeze so
recorded object/archive identities remain valid.
