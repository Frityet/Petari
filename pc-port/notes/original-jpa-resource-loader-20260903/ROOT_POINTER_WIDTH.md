# Original JPA function-pointer allocations

JPAResource::init allocates seven arrays of emitter or particle function pointers. The original four-byte allocation unit and alignment under-allocate those arrays on 64-bit hosts. Each allocation now uses sizeof of its actual function-pointer element type for both size and alignment; dispatch counts and algorithms are unchanged.

The configured original compiler and object comparison report all 37 compared symbols unchanged, including init. Wii pointers remain four bytes. `verify-root.py` reconstructs the original baseline from commit 164ae349c and reproduces the comparison; `root-evidence.json` records the commands/results and `root-pointer-width.patch` is the exact seven-line source correction.

This root checkpoint is independent of the staged native JPC decoder, actual JPA manager ownership, and effect playback. Those are still being integrated and are not claimed working by this allocation proof.
