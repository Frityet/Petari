# Original WarpPod lifecycle and drawing recovery

Recovered the missing `initDraw` and `drawCylinder` bodies and the omitted middle of `initPair` from RMGK01 retail instructions, first in `decomp`, then mirrored into native Game. The texture fields at Wii offsets 0xD4 and 0xD8 are now typed `JUTTexture*`; no substitute geometry, timing, texture, or camera provider was introduced. `initDraw` creates the original 60-point arc and the actual TestColor.bti/TestMask.bti texture owners; `drawCylinder` submits the original two crossed quads per accepted segment using the real camera Z vector and GX API. Its u32 parameter is unused in retail.

Fresh production-source Wii and LLVM23 native object compilation both exit0. `verify.py` reproduces the proof. Original-object comparison:

| Method | Retail bytes | Match |
| --- | ---: | ---: |
| initPair | 348 | 100% |
| initDraw | 992 | 95.91936% |
| drawCylinder | 1516 | 99.93404% |

The draw method has identical call targets/offsets, scalar constant bytes and reviewed non-relocation instructions. Its entire32-byte color table matches the DOL. Remaining initDraw differences are commuted multiply operands, scheduling of independent loop-counter updates, and inlining the existing JUTTexture constructor. The latter was checked against its actual retail owner at0x80181A90: clear embedded palette, storeTIMG, then mask capture flags identically. The resource-name bytes and all initialization branches were reviewed. This is compilation and instruction/data evidence; no live placed WarpPod or tunnel-render result is claimed yet.

## Defined native representation of the original register residue

Retail initPair uses x > then x <, followed by *two y < comparisons* and *two z < comparisons*. When none assigns its local flag, it stores the low byte of r4. This is an original bug and the duplicate comparisons are preserved. The successful getPairPod return at0x80250D4C last loads r4 from paired JMapIdInfo.mZoneID. The return sequence and `_restgpr_29` do not write r4. The no-pair path already faults on the paired position load, so it is not a hidden alternate source for this residual value.

The native-only architecture correction initializes `u8 isPath` from the paired zone ID. Ordinary comparison branches still overwrite it with0 or1. The residual path keeps the full low byte, including values above1; it does not normalize the original flag into a C++ bool. The decomp local remains as recovered and its whole method matches100%. This is the only source-body difference between native and reference. Exact reviewed DOL span hashes are in proof.json.

A runtime fixture for the nonzero residue path requires a real paired WarpPod model/resource and scene manager because the same path constructs the original textures; that fixture has not been added or replaced with mock success. Full scene initialization/drawing remains for the coordinated runtime test.

No root/decomp staging or commits were performed by this lane. Parent owns activation and checkpoint commits. The existing unrelated WarpPod methods were retained; their older nonexact object scores are not evidence of complete runtime parity.
