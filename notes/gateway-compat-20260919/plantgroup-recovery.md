# Original PlantGroup and PlantMember recovery

Recovered the complete retail pair in `decomp/src/Game/MapObj/PlantGroup.cpp`
and its canonical header, following `decomp/AGENT_DECOMP_GUIDE.md`. Exact copies
are in `src/Game/MapObj/`. The already-decompiled `CutBushModelObj` base source
and header are also imported unchanged. The factory integration belongs to the
parent task; this recovery makes no actor-specific compatibility workaround.

The original pair handles CutBushGroup, FlowerGroup and FlowerBlueGroup. It
places individual plants on world collision in concentric rings, aligns their
posture to gravity, manages clipping/appearance as a group, dispatches player
and enemy contacts and spin messages, runs all five original member nerves,
and emits the configured coin or star piece through original item APIs.

## Reference and validation

- Reference assembly: `decomp/build/RMGK01/asm/Game/MapObj/PlantGroup.s`.
- Original text range: `0x8020c7e0..0x8020e394`, all 7,092 bytes covered.
- Retail DOL SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`.
- The split reference object's entire text was compared to that DOL. All
  non-relocation bits match; only bits explicitly covered by the 362 ELF
  relocation entries are masked.
- Original MWCC compile: PASS with the unit's generated Ninja command.
- Objdiff `.text`: **98.22674%**. All methods and five nerve execute methods are
  present. Both actor virtual tables and all five nerve virtual tables: 100%.
- `.sdata2` numeric constants: 100%. `.ctors` and `.sbss`: 100%.
- `.data` layout is 68.09% because of literal/vtable ordering and relocations;
  individual virtual tables compare 100%. `.sdata` differs only by two padding
  bytes after the two Japanese names.
- Four canonical source/header files and port copies compare byte-for-byte.
- `git diff --check` passes for recovered decomp files.

Run `python3 notes/gateway-compat-20260919/verify-plantgroup.py` to reproduce the
isolated Wii compilation, DOL comparison, Objdiff report, and copy checks. The
script does not invoke the native build. Supporting outputs are
`plantgroup-wii-command.json`, `plantgroup-compile.log`,
`plantgroup-match-summary.json`, and `plantgroup-objdiff.json.gz`.

## Functional audit of remaining instruction differences

The largest remaining difference is `initMember` (87.71%): MWCC leaves this
reconstruction's small PlantMember constructor out of line. Its calls use the
same CutBushModelObj constructor, names, model names, light flag and null matrix;
member size/offsets, initial item assignment, item-only shuffle, and sensor
registration agree with the retail instructions. Other remaining differences
are temporary allocation/register ordering in collision placement and posture,
and branch/register scheduling in `tryPush`. These are not missing behaviors.

The numeric audit caught the existing `getEulerDegree` helper's `180.0f / PI`
constant differing from retail by one float ULP. The recovery uses the original
`_180_PI` engine constant after Euler extraction, without changing core math
headers. This makes all numeric constants match retail.

Retail placement divides the accumulated center by the number of successful
collision placements even when that count is zero. This behavior is preserved;
no fabricated collision surface or zero-count fallback was introduced. The
original `tryShake` also returns true outside all three shake radii after its
dead/nerve guards, and that behavior is preserved.

This evidence establishes a complete high-match recovery and exact import. It
does not itself establish native linking, a successful CutBushGroup runtime
placement, rabbit reveal, or Rosalina progression. Those require the parent
task's native integration and original-process runs.

## Repository ownership

The inherited decomp checkout was already at `024901ced` while the parent
gitlink lagged behind it. Updating the parent gitlink to this recovery will
therefore include that inherited clipping-list/view-group commit as well.
Only PlantGroup.cpp and PlantGroup.hpp are staged for this recovery's decomp
commit. The unrelated `NPCUtil.d` and the light agent's LightFunction.cpp work
are preserved.

Published decomp checkpoint: `c1d77e4e4d3560b3844c7d4af2fe541613f4cb54`
(`Recover original plant group and member behavior`). Push to
`origin/pcp-decomp` succeeded, and `git ls-remote` returned that exact SHA.

## Native nerve initialization follow-up

The parent native build exposed duplicate definitions for all five
`sInstance` objects: native `NERVE_DECL` already emits an inline instance.
Replaced explicit definitions with the existing `INIT_NERVE` macro in decomp
first, then copied the source exactly into the port. On Wii the macro expands
to the same original definitions; native expansion uses its existing owner.
The repeated MWCC/Objdiff check is unchanged at 98.22674% text, with 100%
numeric constants, ctor table, and zero-initialized state. No native build was
run by this subtask.

Follow-up decomp checkpoint: `4d771f8cb048f63e55c6f75cd168dec3b67814a2`
(`Use shared nerve instance initialization for plants`), pushed to
`origin/pcp-decomp`; remote SHA verified exactly. Only PlantGroup.cpp was
committed. LightFunction.cpp and NPCUtil.d remain untouched.
