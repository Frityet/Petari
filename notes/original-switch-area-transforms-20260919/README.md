# Original child-zone placement and SwitchCube audit

The actual original process was initializing child-zone objects in zone-local
coordinates. `MR::getJMapInfoTrans`, `getJMapInfoRotate`, and the three rail control
point getters had been simplified to raw JMap reads. Their canonical bodies all
apply the owning `StageDataHolder` matrix when the root has child holders.

`AreaForm.cpp`, `AreaObj.cpp`, and `SwitchArea.cpp` themselves match the canonical
source. Their shape logic was not changed. This correction restores a shared
placement contract for every actor, area and rail which uses these five APIs.

## Retail and actual-owner evidence

The actual Korean disc places `HeavensDoorMysteriousZone` in the root's common
`StageObjInfo` at translation `(14760, -10676.2255859375, 6770)` and Euler rotation
`(65.57914733886719, 70.13896179199219, 56.55928421020508)`. The local WarpPod rows
are `(1580, 538.292724609375, 440)` and `(-1530, -908.0077514648438, -210)`. Before
the fix, those same local positions appeared in actual actor traces.

`read_stage.py` reads the extracted Yaz0/RARC/BCSV metadata without running Game.
`raw-galaxy.json` and `raw-mysterious-zone.json` retain the raw fields; extracted
archives stay under ignored `build/` storage. `extract.log` records the root
archive extraction. These fields are original authored data, not route hints
injected into gameplay.

`verify-retail.py` compiles only the existing canonical JMapUtil unit with MWCC,
without writing dependency files or editing decomp. `JMapUtil-match-summary.json`
records full text 98.57309% fuzzy match; the two placement functions score 100%
and 99.55556%, and each rail getter scores 99.47059%. The complete 4816-byte
reference text agrees with the Korean retail DOL with only the 329 reference
ELF relocation fields masked. DOL SHA1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`. No new decomp commit is needed.

The pre-fix actual-owner test failed exactly at the first transformed row:
returned `(-855.891602, 529.987854, -792.831116)` versus expected world position
`(13894.748, -10278.8203, 7627.56982)`. It separately verified that the raw archive
iterator selected the actual child holder and its nonidentity matrix, so this
was not a fabricated-owner or missing-matrix failure. `placement-red.json/log`
preserve exit1, 1.859s, PID79154 reaped. Initial test compile failure from using
TVec3's nonexistent scalar subscript is retained in `red-build.log`; the fixture
was corrected to explicit component pointers, not a production vector change.

## Changes

- `src/Game/Util/JMapUtil.cpp`: restored only the five complete original function
  bodies and required includes. No gameplay defaults or special stage cases.
  `native-donor-body-equivalence.json` checks complete bodies with only whitespace
  normalized.
- `StagePlacementResolver.cpp`: native inventory rows, child rows, retained rail
  control points and StartInfo copies now keep authored local values. Descriptor
  translation/rotation/world bases still explicitly receive the zone transform.
  Removed the eager JMap float-override helper and its public declaration.
- `StageResourceBinding.cpp`: general-position catalog copies retain local fields
  as well. Actual original-process owner lookup remains in original SceneUtil;
  no synthetic fallback was added.

This removes the historical assumption that native tools should bake world
coordinates into copied JMap fields. The inventory descriptor and the borrowed
original row are different representations with explicit coordinate contracts.

## Validation

`OriginalProcessPlacementTransformTests.cpp` starts the ordinary original
GameSystem with actual disc assets and fresh console settings. The test observes
an initialized GameScene, reads its actual StageDataHolder rows, and writes no
Game positions, flags, switches, nerves or ownership.

`placement-green.json/log`: PASS120 completed frames, exit0, 3.650s, PID80089
reaped. At frame36 the test checked 84 translations, 84 rotations, 48 rail control
points, both actual SwitchCube forms and both ordinary WarpPod actors. It checks
rotation composition with independent double-precision trigonometry and a small
allowance for the original short-angle table, tests the real cube center and a
point beyond its side, and verifies normal scene retirement. Inventory and start
copies are also checked against their separate world metadata, preventing a
second transform from being baked into retained rows.

The existing pure rigid-zone/StartInfo case in StageStartCameraTests now requires
retained position and rotation fields to remain local, while its previously
asserted world position/front/up stay transformed. `--placement-only` runs this
owner-free descriptor test independently of older synthetic camera fixtures.
`pure-start-test.log` records its result. The broader old camera fixture suite was
not claimed as validated; many of those fixtures predate actual GameSystem
ownership. No owner checks were weakened.

Root's independently verified restoration of ModelObj::init to the original
appearance-only body was included in the green build. CrystalCage and StarPiece
focused targets were linked in the same coherent source snapshot, but their
separate owner/action claims are recorded in their own notes.

The 120-frame read-only test proves placement and retirement. It does not prove
rabbit capture, SwitchArea activation by the player, pipe travel, or Rosalina
progression. A fresh ordinary gameplay run is required after the main relink.

## Original SwitchCube contract for later gameplay diagnosis

The ordinary `SwitchCube` factory constructs `AreaForm::Type_Cube2`: a cube with
its origin at the center of its bottom face. Its local bounds are
`[-500*sx, +500*sx)`, `[0, 1000*sy)`, `[-500*sz, +500*sz)`. Rotation and translation
form the rigid matrix; scale affects these bounds. An optional follow matrix
left-multiplies that matrix. Membership uses its transpose rotation after
subtracting translation, then inclusive lower/exclusive upper comparisons.
AreaObj additionally requires its valid, follow-valid and awake flags.

Both authored reveal cubes are in MysteriousZone/common/AreaObjInfo:

| l_id | local position | local scale | A | B |
| --- | --- | --- | --- | --- |
| 8 | (1550, -760.6881103515625, -1090) | (0.3,0.3,0.3) | 1113 | 1111 |
| 11 | (-670, 377.11883544921875, -590) | (0.9,0.9,0.9) | 1114 | 1111 |

All eight arguments, Appear, Sleep and FollowId are -1. SwitchArea therefore
latches A on only after B is on and the actual player's base position is inside
the volume; there is no ground requirement and leaving does not turn A back off.
Switch1111 is the RunawayTico startRunaway gate; A1113/1114 drive the corresponding
rabbit appearance links. IDs >=1000 use the actual global switch container with
index ID-1000. Merely approaching a bush or attacking it does not replace this
original volume/switch condition. SwitchWatcherHolder runs at movement category
0x1B after the area category0x0D and propagates actual switch edges.

Any previously recorded raw-coordinate operator waypoint must be transformed
through the current actual form/zone matrix after this correction. No waypoint
or actor-specific navigation behavior was added to Game.

## Main handoff

After the placement test, pure descriptor test, and both corrected CrystalCage/
StarPiece 120-frame probes passed, the main relink passed in6.003s.
`main-build.json` records SHA256
`b38e90868eb8ab1f1cdc72487c293cf0f15a3e50c6fd8b14adc846bec7a7d123`.
The executable and source were released to root for a fresh ordinary gameplay
run; no ongoing process or debugger remained owned by this subtask.
