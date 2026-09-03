# Original alpha-buffer clearing

Recovered both missing MR::clearAlphaBuffer overloads in root DrawUtil.cpp, then copied their complete bodies into the native OriginalAlphaClear provider. The full-screen overload reads the actual JUTVideo framebuffer dimensions. The rectangle overload preserves unsigned-16-bit dimensions, the original orthographic projection, six triangle vertices, alpha-only writes, destination-alpha override and final clip-mode restoration. It does not clear RGB or replace the game operation with a host framebuffer clear.

The original GC3.0a3 build succeeds. Full-screen wrapper: 120 bytes, 98.167% match; rectangle: 744 bytes, 99.758% match. Both preserve every direct call in retail order. The evidence records relocation constants, function hashes and exact compile command. This is source/compiler proof, not an executed shadow or alpha-clear GPU claim.

Run `python3 pc-port/notes/original-alpha-clear-20260903/verify.py` from the repository root. It verifies the source/native body identity and compares against the original split object and verified DOL.

The native geometry header also imports the existing original three-vector TPosition3::setPositionFromLookAt body, required by CollisionShadow. That import changes no Game algorithm.
