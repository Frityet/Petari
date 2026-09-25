# DirectDraw GX texture tiling

Implemented the missing TDDraw::tileConversion8 and tileConversion16 in their actual Game/Util/DirectDraw.cpp owner. No alternate provider, build wiring or fixture was added. Header documents the native input / GX-byte output contract.

8-bit conversion retains the original 8x4 block order and four-row-band copy, with local scratch sized to the actual width instead of the shared 0x800-byte buffer. 16-bit conversion visits 4x4 blocks, preserves both bytes of each native u16 pixel, emits high then low byte, and copies the complete width*8-byte band. Each band is fully read before in-place overwrite; scratch is local to the invocation. Zero dimensions are no-ops. Invalid pointers, incomplete blocks and native-size overflow are rejected because this in-place API cannot infer storage for padded edge blocks. Current callers use complete blocks.

## Authoritative retail discrepancy

The existing donor is not merely mistranscribed: retail disassembly corroborates its broken 16-bit implementation. Saved retail-tiling.s records both functions from decomp/build/original-material-controllers-20260903/retail/asm/Game/Util/DirectDraw.s. At 0x80403F44 it loads a halfword; 0x80403F48 stores one byte; scratch advances one byte per texel. At 0x80403F74 it requests a constant 0x20-byte copy; destination advances by the same constant per four-row band. Therefore this native 16-bit implementation is an intentional generic correctness change requested by the coordinator, not a claim of retail matching or new decompilation. The decomp source remains unchanged.

## Callers and byte order

SnowFloor/SnowFloorTile fill row-major byte intensity images before conversion and flush their full images afterwards. NormalMapBase::createGradTexture fills native u16 IA8 values (low-byte intensity, high-byte alpha), calls tileConversion16, then flushes width*height*2 bytes. Aurora TextureDecoderIA8 reads those physical [alpha,intensity] bytes as a little-endian halfword and selects intensity from the high native byte; RGB565/RGB5A3 decoders also expect original big-endian halfwords. Emitting GX bytes therefore preserves the intended texture representation across these formats.

Read-only checks used the existing decomp guide, retail instruction listing, actual callers and Aurora texture decoder. No build, runtime, tests, index or Git commands were run. Runtime/pixel validation is not claimed. Exact before/after snapshots and scoped patch are included.
