# Original fur shader recovery, 2026-09-07

Recovered every missing CShader method in decomp first, following
AGENT_DECOMP_GUIDE.md, then copied the complete source into native Game. Native
FurShader remains explicitly excluded from the normal Game archive while the
parent coordinates the whole FurCtrl/Multi/Drawer ownership graph.

The retail shader is a CPU displacement pass. It swaps the model's actual
transformed-position buffers, reads current positions/normals and authored UVs,
normalizes the selected normal, and displaces by the sampled fur length times
_1C. A zero length-map texel retracts the vertex by one normal unit. Fixed16 UVs
use the original 0.45-biased integer span normalization and negative-coordinate
adjustment; float UVs sample directly. Output keeps the original S16/F32 format,
flushes its real range and becomes the current model position buffer. Existing
J3dGeometryData already converts CPU vertex scalar arrays to host values.

makeIndexData parses the actual shape display lists and builds position-index
to normal/UV-index mappings. checkBorderVtx invalidates vertices referenced by
other shapes unless their actual material name contains the original "Fur"
substring. No actor-specific material or model exception was added.

CLengthMap now has a typed byte pointer and f32 sampling result. The original
constructor points at ResTIMG+0x20; invalid/missing/non-I8 maps return unit length
through the original disabled flag. I8 samples address 8x4 tiles: block index is
x/8 + (width/8)*(y/4), then local byte x%8 + 8*(y%4), divided by255. Original
getTexelOrder behavior is preserved exactly, including its existing repeat-like
mirror case and conditional handling only above1. These behaviors must not be
"corrected" to a new generic sampler in Game.

CShader now derives the actual J3DUnkCalc1 callback interface. The SDK owner lane
made its original calc/setup slots pure virtual, with no base destructor slot;
CShader's own calc/setup/destructor table still matches retail100%. No fake
callback adapter or alternate model slot is introduced.

Native source differences are architecture-only: include aurora/endian.hpp and
replace six unaligned big-endian u16 loads from GX display lists with the shared
Aurora read_u16 helper. I8 bytes and already-converted CPU scalar arrays require
no per-shader copy or endian conversion. Header is identical between native and
decomp. Shared fixed16 conversion providers are owned by the parent.

Fresh configured MW compilation and objdiff pass. calc90.14%, index mapping91.12%,
border masking95.54%; all other recovered/preserved methods99.31–100% except
sampling refer84.64%, whose remaining register/instruction ordering differs.
The coordinate returns stay signed until both original calls finish, then narrow
to u16 before unsigned tile quotients and signed-promoted local modulo. This
recovers the retail evaluation order and improves the initial70% candidate.
The exact I8 addressing and scalar operations were checked against retail;
independent native behavioral tests are being written by the other lane.
Fresh PPC layout assertions also verify CShader0x28, CLengthMap0xC, CIndex4,
and both pointer/flag offsets. Freshly recompiled pre-change source shows no existing-function score regression;
CLengthMap constructor improves96→100. Native LLVM23 isolated syntax passes.
Commands, source hashes, baseline and object comparisons are retained here.
No inline assembly was added. Native execution/whole fur rendering is pending.

Owned source paths:
- decomp/src/Game/Util/FurShader.cpp
- decomp/include/Game/Util/FurShader.hpp
- src/Game/Util/FurShader.cpp
- src/Game/Util/FurShader.hpp
- src/Game/xmake.lua: temporary FurShader exclusion only

The gateway lane owns J3DUnkCalc1 SDK headers, and the parent owns the shared
Aurora endian header. No staging or commits were performed independently.
