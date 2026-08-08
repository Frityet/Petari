# Mii / RFL runtime audit

Date: 2026-08-08

## Verdict

The exact `FileSelectItem` / `FileSelector` Mii path must remain absent on PC.

The retail `RFL_Res.dat` can now be copied, structurally validated, indexed, and
owned by Aurora. That proves resource acquisition and lifetime only. It does
not prove character construction or rendering. In particular, the current PC
RFL surface still has no real implementation of:

- default or official `RFLiCharInfo` acquisition for a model;
- shape and texture resource decoding into a model;
- normal and blink mask composition;
- model matrix and expression mutation;
- opaque or translucent draw submission;
- icon rendering;
- the exact `MiiFacePartsHolder` construction, scheduling, and destruction
  path.

Consequently, `FileSelector` and `FileSelectItem` remain excluded, their name
factory entry remains unavailable, and no fake face, placeholder icon, null
actor, no-op draw, or event-only substitute was introduced. The pre-existing
`MiiFacePartsHolderCompat.cpp` null/no-op implementation and its scene-object
factory case were removed during integration. A focused scene-object test now
requires the Mii holder to remain absent.

## Implemented honest foundation

Aurora now owns `aurora::rfl::ResourceArchive`. It:

- copies the caller's bytes instead of borrowing the RARC buffer;
- validates the 18 retail subarchive headers and big-endian offset tables;
- rejects zero versions, empty sections, invalid ranges, non-monotonic file
  offsets, mismatched largest-file sizes, and invalid data extents;
- exposes stable section metadata and bounded file spans;
- releases the owned bytes when `RFLExit` resets `RflService`.

`RflService::init_resources` delegates this format work to Aurora. It reports a
malformed resource as `RFLErrcode_Broken`. Resource/model creation and icon
calls with return values report `NotAvailable` until those operations are real.
The unsupported void model/draw ABI (`RFLSetMtx`, expression access, draw-state
setup, and opaque/translucent submission) has no PC definition at all: because
the exact Mii actors are excluded, accidental activation now fails at link time
instead of reaching a silent stub.
Resource and NAND-database status are invalidated independently: a malformed
resource attempt can be retried with the real archive on the same service, and
the valid owned archive replaces the cached resource error without inventing a
database record. The focused suite locks this invalid-to-valid retry sequence.

The exact recovered PC mirrors were added for:

- `MiiFaceParts.hpp` / `.cpp`;
- `MiiFaceRecipe.hpp` / `.cpp`.

They are byte-identical to the decomp source and protected by
`GameSourceMirrorTests`. They are deliberately excluded from the PC target
until their compatibility dependencies and the real RFL runtime exist.

The root and PC `MiiFacePartsHolder.cpp` mirrors were also advanced together.
`drawExtra()` was recovered from RMGK02 and is now a 100% function match. Its
TEV register selection, stage inputs, light masks, and depth mode were corrected
from the target assembly. This improves the authoritative source; it does not
activate the holder on PC.

## Smallest retail closure for exact file select

The actual ownership and call chain is:

1. The scene owns one `MiiFacePartsHolder` and its 32-byte-aligned RFL manager
   work buffer until scene teardown.
2. Aurora owns a validated copy of `RFL_Res.dat` for the same RFL generation.
3. `RFLInitResAsync` transitions from `Busy` to `Success` only after that
   resource is ready.
4. Every `FileSelectItem` creates a default index-zero face first. A saved Mii
   later changes the recipe to an official database index. Therefore a real
   retail default record and model are mandatory even when `RFL_DB.dat` is
   absent.
5. Every `MiiFaceParts` owns its recipe, a 32-byte-aligned model buffer, an RFL
   model handle, and its fixed-position object. The holder owns registration,
   initialization order, animation/view forwarding, and draw traversal.
6. Model construction must decode the selected retail record and the exact
   shape/texture files, generate normal and blink RGB5A3 masks, and retain all
   resulting geometry/texture state until the actor is destroyed or rebuilt.
7. `RFLSetMtx` and `RFLSetExpression` must mutate that live model. Opaque and
   translucent calls must submit the real display lists through Aurora GX.
8. Only after creation, async completion, matrix update, expression change,
   opaque draw, translucent draw, rebuild, and teardown are tested may the
   holder and `FileSelector` be activated.

The detailed implementation order and host compile blockers are in
`remaining-runtime-closure.md` and `host-compile-audit.log`.

## Verification

All checks were green at handoff:

```text
ninja -j4
build/tools/dtk shasum -c config/RMGK02/build.sha1
  build/RMGK02/main.dol: OK

xmake -j4 smg-pc-rfl-real-or-absent-tests
xmake run smg-pc-rfl-real-or-absent-tests
  4 RFL real-or-absent tests passed
  includes cached-status malformed -> valid resource retry

xmake -j4 smg-pc-sceneobj-holder-real-or-absent-tests
xmake run smg-pc-sceneobj-holder-real-or-absent-tests
  3 SceneObjHolder real-or-absent tests passed

xmake -j4 smg-pc-rfl-resource-archive-tests
xmake run smg-pc-rfl-resource-archive-tests
  version=925 bytes=686372 files=472
  fnv1a=0x5659aa5c5532e566

xmake -j4 smg-pc-game-source-mirror-tests
xmake run smg-pc-game-source-mirror-tests
  all source pairs passed, including all six Mii parts/recipe/holder pairs
```

No files were staged or committed as part of this audit.
