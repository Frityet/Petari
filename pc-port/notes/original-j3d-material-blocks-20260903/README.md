# Original material blocks and resource prerequisites

This checkpoint supplies the complete original J3D material block source,
original block factories and allocation-size methods, and remaining TEV helper
providers. It supports the actual material-animation value objects restored in
`../original-material-animation-20260903`. The related joint and collision
resource components retain real typed backing storage for original SDK/Game
objects. These are prerequisites for the actual model/animation and floor
owners required by Mario's original jump path. Jumping is not activated here.

The animation timing correction from `949555f46` remains active: simulation
runs at60 logical ticks per second independently of presentation. There is no
animation-rate multiplier in these changes.

## Original source and architecture boundaries

`verify.py` builds the actual root J3DMatBlock, J3DMaterial, J3DStruct and
J3DTevs units with the configured GC3.0a3 flags. It records per-function
comparisons in `compiler-evidence.json`, verifies native source correspondence,
and compares14 default objects to raw RMGK01 DOL data. Two further zero defaults
are in retail BSS and are identified explicitly as such.

The root decompilation gains the original light load, GXColorS10 assignment,
texture-coordinate assignment, indirect texture-matrix assignment, fog
assignment, NBT scale assignment and inline depth-state writer. Four of these
compile at100%. Fog assignment is88.96875% with the same128-byte extent: register
assignment and independent load/store scheduling differ. The depth writer is70%
with the same48-byte extent: the same OR/shift graph and FIFO writes are scheduled
through different registers. The indirect matrix copy is a functional C++
translation of three paired loads/stores plus the exponent byte; its scalar
60-byte compile has0% instruction matching against36-byte retail paired code.
No new inline assembly was added to obtain a cosmetic percentage.

The indirect copy explicitly loads all six float components and the exponent
before writing them, preserving self-assignment and the three padding bytes.
Fog retains all typed components and10 adjustment coefficients. NBT preserves
its three padding bytes. Texture-coordinate assignment updates the original
four-byte information record while preserving its cached matrix register.
The field boundaries and native behavior are checked in the material fixture.

The original default texture matrix was incorrectly missing its effect matrix
initializer in the root decompilation. Raw data at8055C250 proves it is identity,
not zero. Root and native constants are corrected; the full100-byte original
constant now matches retail, and the actual native constructor has an explicit
identity regression. A duplicate inline BP writer in root MatBlock was removed
because the recovered shared GD header already defines it.

Native changes are confined to SDK compatibility boundaries:

- Packed RGBA colors and packed TEV stage byte fields are read in big-endian
  order before becoming numeric GD words. Texture-number patches use the same
  helper for unaligned BP payloads. Host-endian word casts would reverse these
  bytes on Apple Silicon.
- The fog-range provider takes the declared unsigned-byte parameter. Aurora's
  C++ GXBool is bool, while the original ABI uses u8; using GXBool in this
  provider silently created a different overload and left the real call unlinked.
- Original block allocation-size methods use sizeof and therefore account for
  native pointer width. Material behavior and factory selection are unchanged.

## Validation and remaining work

The shared build covers ten CPU fixtures and the showcase. `native-gates.json`
records exact executable hashes, return codes and outputs. All ten pass; real
disc input is enabled for the joint and transform-animation resource groups.
Mario's BDL provides30 joints,13 envelopes,48 serialized draw entries,35 original
draw matrices and22 full-weight matrices. Wait, Run and Jump BCK resources are
also exercised. No BCA is authored in that animation archive; its native
sampling coverage remains the explicit byte fixture.

The new material fixture has five groups: original factory selection; RGBA
commands and bounded patching; packed TEV arithmetic and unaligned texture
indices; depth and fog commands; and the recovered structure assignments. The
material-animation fixture has five groups. The collision fixture has ten
groups and separate ASan/UBSan evidence in its own notes. `runtime-gates.json`
records fresh title and Gateway smoke runs against the supplied disc.

This checkpoint does not yet publish a partially loaded model as a complete
original J3DModelData. Material/shapes/texture backing, the original ResourceHolder
lifecycle, renderer ownership and original Mario floor/state integration remain
to be connected. Raw GD texture/TLUT commands also need Aurora's hardware
address and palette semantics; their fixed command sizes and original patch
offsets must be preserved. The current known grounded KCL-seam issue and jump
activation are not claimed fixed by these resource tests.
