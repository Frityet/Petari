# Original material animation buffer and SDK owners

This tranche restores root `Game/Animation/MaterialAnmBuffer.cpp`, its actual
field type/declarations, and the missing SDK material-color model forwarding
method. PC Game receives the recovered buffer unchanged. Dedicated native SDK
providers execute the original material-animation value objects and four
resource families. No ResourceHolder, ModelManager, MarioAnimator, or Game
player owner is activated here.

## Retail recovery and proof

The source oracle is the verified RMGK01 DOL at
`build/compat-math-oracle/main.dol`, SHA1
`25c5959534b3c21246c6c7e42021b916b41fb578`. The original compiler is GC3.0a3
using the actual configured Game flags for the buffer and SDK flags for the
JSystem translation units. There are no synthetic include overlays or fake
virtual methods. `verify.py` recompiles the real root files, compares extracted
retail objects, and verifies each declared native adaptation.

| Recovered buffer method | Address | Retail size | Measured match |
| --- | --- | --- | --- |
| Constructor | `800183F4` | `CC` | 100% |
| `getDiffFlag` | `800184C0` | `10` | 100% |
| `getAllocMaterialAnmNum` | `800184D0` | `14` | 100% |
| `searchUpdateMaterialID` | `800184E4` | `F4` | 98.03278% |
| `setDiffFlag` | `800185D8` | `17C` | 98.26316% |
| `getDifferedMaterialNum` | `80018754` | `44` | 100% |
| `attachMaterialAnmBuffer` | `80018798` | `5C` | 100% |

All seven have the same complete function size as retail. The two 98% functions
retain the exact four-table order, resource/name queries, target search calls,
and flag calls. Their remaining differences are register allocation across the
table loops and local constant symbol names. The eight MR on/off wrappers,
three `modifyDiffFlag` template instantiations, and `modifyDiffFlagBrk` cover
`800187F4..80018A78`: templates and BRK functions are100%; BPK/BTP/BTK wrappers
are99% because of their local string symbols. The actual DOL strings at
`806B1640` are exactly `bpk\0btp\0btk\0`.

The previously missing
`J3DAnmColor::searchUpdateMaterialID(J3DModelData*)`, `80436140` size8, now
forwards to the actual embedded material table. Its original-compiler match is
100%, as are the analogous existing sibling methods. A declaration was added
to the original and native Animation headers without changing layout.

All six `JUTNameTab` methods and its generated destructor are100%. Original
material-animation `initialize` and `calc` are100%. Their five setters measure
69.375% (52/64 bytes): GC outlines the existing value-object assignment into a
tail call, whereas retail inlines its pointer/ID/flag field stores. The null
branch and actual copied fields agree; source algorithms were not altered to
chase the percentage.

The material resource constructors, all name lookup methods, texture-SRT
calculation, full-color sampler, and texture-pattern sampler are100%. The three
key-color/register samplers measure88.295456% (656/704 bytes). Retail materializes
the returned float in stack storage and calls `OSf32tou8`/`OSf32tos16`; this
compile outlines `__OSf32tou8`/`__OSf32tos16` instead and stores the returned
integer. The same channel switch, interpolation call, finite clamps, and scalar
conversion remain. These existing SDK source imports are functionally traced;
the lower percentages are explicitly not claims of exact matching.

Full per-function results, source hashes, exact commands, and retail function
hashes are in `compiler-evidence.json` and `original-commands.json`. The raw
objects/disassemblies are ignored under `build/original-material-animation-20260903`
and the previously extracted retail object directory. The initial whole buffer
disassembly is also retained under the preceding shape-packet audit build folder.

## Original allocation and attachment contract

`MaterialAnmBuffer` keeps its existing `_0` animation-array field and restores
`_4` from `void*` to the proven `u32*` difference-flag array. Its constructor
always resolves material names in BPK, BTP, BTK, BRK order. With selective
allocation enabled it allocates and zeroes one word per material, sets the
original masks, counts nonzero entries, then allocates that many genuine
`J3DMaterialAnm` objects. With selective allocation disabled, it allocates one
animation object per material and leaves `_4` null. Attachment uses the actual
material table and assigns consecutive animation objects only to selected
materials. It reloads `_0` within the original loop; no alternate pointer-owner
or virtual-table scheme is introduced.

The original masks are BPK=`1`, BTP=`0x20000`, BTK=`0x200`, and
BRK=`0x1000000`. Both BRK color and konst lists set the same TEV-register bit.
Missing target ID `0xFFFF` is skipped; clearing one family preserves all other
bits. BTK target count is its track count divided by3. Names passed to these
helpers are retained in the original signatures even though the release
implementation does not read them.

`getDiffFlag` assumes the flag array was allocated. No null-return shortcut or
unconditional allocation was added. The original class has no destructor;
future native resource ownership must retain and explicitly release its
arrays under the correct resource lifetime. Name searches also mutate each
animation object's actual update-ID arrays, so an owner must account for that
when sharing authored resources between models.

## Native source boundaries

- `Game/Animation/MaterialAnmBuffer.{cpp,hpp}` is byte-identical to root.
- `compat/J3DMaterialAnmCompat.cpp` and `compat/JUTNameTabCompat.cpp` are complete
  byte-identical copies of their existing root source units.
- `compat/J3DMaterialAnimationCompat.cpp` imports ColorKey/ColorFull, TexPattern,
  TextureSRTKey, and TevRegKey constructors, material searches, and samplers.
  Native substitutions use the existing PPC integer conversion/rotation-shift
  boundary for full-color frame indices, texture-pattern indices, and BTK
  rotation. No sampling curve or frame rule is replaced.
- `compat/J3DAnimationInterpolation.hpp` contains the existing native Hermite,
  key lookup, and PPC integer helpers moved unchanged from the transform sampler.
  Both samplers include that header after disabling implicit FP contraction.
  The existing transform verifier now reads the shared header too; the full
  original transform correspondence and signed16 retail arithmetic graph still
  pass. Explicit Hermite FMA ordering remains intact.
- Full original material factory/block support comes from the parent tranche.
  The new fixture uses those actual classes, not stand-in virtual implementations.

The pre-existing Aurora `OSFastCast.h` uses native casts for PSQ conversions.
These tests exercise finite values within the original clamps; NaN behavior is
not validated here. No assertion of complete PSQ nonfinite conversion parity is
made. The source-backed integer conversion helpers used for frame/rotation
already define the separate original `fctiwz` boundary.

## Validation and the remaining owner dependency

All five affected native source translation units and the new
`tests/OriginalMaterialAnimationTests.cpp` passed isolated native compiler
probes using the real compile database. Parent owns the shared build, target
wiring, and execution; this worker has not run a shared build.

The fixture has five groups using genuinely constructed original objects:
material name search including missing and post/konst lists; every difference
mask with invalid IDs and preserved unrelated bits; finite color/TEV Hermite
clamps and signed truncation; original BTK rotation and full-color/BTP frame
differences; and real `J3DMaterialAnm` initialization, copied bindings, material
block updates, and null-binding disable behavior. Name tables are actual
host-endian `ResNTAB` value resources with retained strings; no raw retail
big-endian table is passed directly to a native SDK object.

The complete buffer constructor is **not runtime-tested** yet because the PC
archive-only global `ResourceHolder` in `compat/ResourceHolderCompat.hpp` remains
incompatible with the actual Game class. The fixture deliberately does not
construct a fake holder or an unconstructed buffer. That global class conflict,
typed model/material/resource loading, and authored lifetime ownership remain
the next actual ResourceHolder integration work. The now-linked buffer and SDK
objects close real prerequisites without pretending that owner already exists.

Run `python3 pc-port/notes/original-material-animation-20260903/verify.py` to
reproduce the root compiler and correspondence results. It writes fresh results
to ignored build storage and does not modify shared build configuration.
