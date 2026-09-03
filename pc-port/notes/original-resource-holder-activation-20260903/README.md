# Original ResourceHolder prerequisites, 2026-09-03

This checkpoint closes two general prerequisites for constructing the original
ResourceHolder: a stable source identity for JMapInfo::attach and actual
PSMTX44Copy/Identity entry points. It does not yet activate ResourceHolder's
constructor or replace the archive-only global wrapper.

## JMap source and cache ownership

JMapResource already retained a decoded table for its own copied byte address.
The original BckCtrl constructor instead attaches the byte pointer returned by
JKRArchive. `register_source(span)` now validates the complete byte extent and
identity before exposing that borrowed pointer to the original unsized attach.
Its movable RAII registration retains the resource, supports nested registration
by the same owner, rejects a conflicting owner, and removes exactly its own
registry generation when the last alias is released. An attached reader retains
its shared table independently of a later alias removal. Registering the owned
byte pointer itself does not remove its baseline registration.

The archive owner must retain its borrowed byte range until its registration
ends. Construction does not silently register a caller's temporary input span.
This is the same explicit ownership boundary used by the animation/model owners.

A separate actual heap test found a concrete bug: a first string read inside
JkrAllocationScope allocated the shared lazy string cache in the selected Game
heap, while the JMap resource could outlive that heap. JMapResource's parser,
registry and storage, JMapInfo's native table constructor/from_bcsv, and the lazy
string lookup now escape Game allocation routing. Their shared data remains host
owned. Repeated reads still return the same character pointer; they never replace
an existing cached string. The root JMapInfo algorithm was not changed; this
native class remains the existing bounded BCSV compatibility implementation.

`verify-jmap.py` builds the actual JKR heap, OS mutex/scheduler, MSL and JMap
providers. All **7/7 groups pass** in optimized, ASan/UBSan, and TSan builds:
local reader lifetime, attached reader lifetime, independent rebind ownership,
concurrent readers, nested archive aliases, identity/extent validation, and
shared cache survival after the heap selected for its first read is destroyed.
The same new heap test against the prior unescaped JMapInfo provider failed the
ownership assertion; that negative control safely destroyed the cache before
retiring its heap. Scratch binaries remain ignored.

## Actual matrix entry points

The authoritative SDK already has the two routines as original assembly.
`src/RVL_SDK/mtx/mtx44.c` now retains those instructions under __MWERKS__ and
provides their portable branch for native compilers. Aurora mirrors those two
bodies in its existing mtx44.c. No Game algorithm uses a replacement matrix path.
Both SDK headers expose the actual PS-prefixed entry points on native targets.
Other unsupported mtx44 extension functions are outside this change.

The copy performs eight forward pair loads/stores, including in-place copies.
With the original SDK's GQR0=0 convention, the quantized stores flush subnormal
floats to signed zero. Ordinary values, infinities, and NaN payload bits survive.
A whole-matrix memcpy/memmove or an early in-place return would not reproduce all
of these observable memory results. The native code uses memcpy only for each
8-byte pair to avoid C strict-aliasing or signed-representation issues.

`verify-mtx44.py` verifies the supplied DOL SHA1, actual r2 initialization and the
two addressed constants. The configured original compiler produces exactly the
retail bytes for Identity at **0x804B8F74 / 0x34** and Copy at
**0x804B8FA8 / 0x44**, after the two verified SDA relocations. It interprets the
actual retail instruction words and compares **12,000** random 32-bit-pattern
copy/overlap cases and an identity case against the compiled Aurora provider.
The bit-level load/store oracle follows Dolphin's hardware-tested conversion
routines, independently of the native exponent-mask reduction.

The four standalone CPU groups pass optimized and ASan/UBSan: full identity and
bounds, special-value/in-place copy, forward paired overlap, and reverse overlap.
The real isolated Aurora CMake target `mtx44_tests` also builds successfully,
and `ctest -R '^mtx44$'` passes its one executable containing all four groups.
The CMake build/test logs are retained here. No shared xmake build or GPU run
was performed by this worker for this checkpoint. The proof concerns the matrix memory results;
PPC FPSCR flags/traps and arbitrary nonzero GQR0 configurations are not emulated.

## Original ResourceHolder closure audit

Current root ResourceInfo has fifteen complete methods; the native file already
contains their same bodies. Current root ResourceHolder has thirteen complete
methods. Fresh configured GC3.0a3 comparisons are **100% for all 28 methods**;
there is no missing resource-table routine to recover. This percentage describes
the functions, not unrelated data-section pointer relocation labels.

`verify-resource-tables.py` also extracts the unchanged original static count and
getFileFinder bodies for an isolated fixture. It links the real JKRMemArchive,
original finder, actual heap, and MSL formatting providers. No ResourceHolder is
constructed and no partial model/holder owner is substituted. ASan/UBSan passes
recursive null-root-path traversal, extension matching and an empty directory.
The supplied MarioAnime archive also passes **574 total files, 456 BCK, 1 BANMT**.

The complete original ResourceHolder source compiles against the current native
headers after the actual PS matrix declarations are available. Remaining link
and ownership work is concrete:

- Complete retained J3DModelLoaderDataBase model/BDL/material-table dispatch.
  The parent's J3dModelResource contract provides `register_source`, `load(flags)`
  and `load_material_table()` with actual original objects and a retained JKR
  allocation domain plus Mem1ResourceHeap.
- Copy original ModelUtil post-load routines and their source helper closure:
  initEnvelopeAndEnvMapOrProjMapModelData, downFracVtx, isUseFur, isEnvelope,
  isUseTexMtx/EnvMap/ProjMap, and setShapeVcdVatCmdSelf. These bodies already exist
  in root. They mutate the loaded original model, so the model owner must retain
  mutable typed vertex/shape/material storage and the real display-list data.
- Construct a real JKRMemArchive and register each supported animation, model,
  BMT and BANMT identity before running the original whole constructor under an
  actual retained allocation domain. MarioAnime itself includes one BMT, so an
  animation-only owner is insufficient even for that archive.
- Replace the global archive-wrapper class atomically with actual Game
  ResourceHolder. Native collision diagnostics need a separate service mapping
  from the actual holder to the retained raw archive/path. MR's original return
  signatures must continue to name only the actual Game class.

Current archive inventory: Mario.arc has one BDL and one BTI. MarioAnime.arc has
one BANMT, 39 BAS, 456 BCK, one BCSV, one BMT, one BTI, four BTK, and 71 BTP.
The already completed animation owner handles the typed J3D animation families;
BAS/BTI/general files stay raw resources as original ResourceHolder specifies.

Lifecycle order must retain the archive, native typed backing, source aliases,
actual loaded SDK objects and the allocation domain for all borrowed resources.
Destroy SDK objects while their domain remains live, then release native owners
and the heap. ResourceHolder's shallow implicit destructor alone does not own
all table/material-buffer allocations; the retained real heap supplies their
arena lifetime. This is preparation for the atomic migration, not a claim that
original Mario jump/animation owners are active.

## Reproduce

```sh
python3 pc-port/notes/original-resource-holder-activation-20260903/verify-jmap.py
python3 pc-port/notes/original-resource-holder-activation-20260903/verify-mtx44.py
python3 pc-port/notes/original-resource-holder-activation-20260903/verify-resource-tables.py
cmake --build build/aurora-upstream-merge-tests --target mtx44_tests -j 4
ctest --test-dir build/aurora-upstream-merge-tests -R '^mtx44$' --output-on-failure
```

The last script requires the prior archive proof's dtk split objects. It runs
its real archive check when the locally supplied extracted MarioAnime.arc is
present. Retail binaries and native build artifacts stay under ignored build/.
