# Independent original FurShader checks — 2026-09-07

The production FurShader recovery belongs to the sibling agent; this scope adds an independent native regression and read-only retail audit. `tests/OriginalFurShaderTests.cpp` calls the actual CShader/CLengthMap methods from the normal Game archive. It does not substitute shader callbacks, bypass private shape fields, or alter production code.

Five cases cover:

- I8 sampling across all four 8x4 tiles of a 16x8 image, exact endpoints, and sampling disabled by null/unsupported maps. Direct getTexelOrder checks preserve the original negative-coordinate behavior without feeding invalid negative coordinates into the original sampler. Repeat and mirror both reduce only inputs initially greater than one.
- Big-endian raw GX command counts and position/normal/UV indices with deliberately unaligned reads, direct matrix attributes, seven-byte vertex stride, multiple commands, later mapping overwrite, untouched sentinels and missing-UV early exit.
- Border exclusion through actual original shapes constructed and retained by J3dGeometryData from a bounded SHP1/VTX1/INF1 fixture. Current-shape and other-Fur mappings remain; a shared ordinary-material vertex loses both mappings. A shape without a position descriptor is ignored. Material names use the real decoded J3dNameData owner.
- Float displacement through the actual J3DUnkCalc1 virtual interface and J3DModel vertex buffer. Normal indices are distinct from positions; normals normalize before zero-mask one-unit retraction or full/partial `_1C` displacement. Invalid mappings leave destination values unchanged; repeated calls alternate actual buffers and read current deformed positions.
- Signed fixed positions and normals with separate fractional precision, original integer UV range calculation, negative UV wrapping and output conversion. The negative `-1/3 + 1` path rounds to a texel immediately below positive `2/3`; the assertion deliberately preserves this float behavior.

`independent-retail-audit.json` records the actual retail disassembly source/hash, function addresses and checked operations. Sampling layout, wrap branches, index widths/order, border filtering, displacement and virtual slot order agree with the reconstructed source. No independent functional discrepancy was identified within those checked paths. The production recovery's numerical object-match evidence is maintained separately in `../original-fur-shader-20260907/`.

Final `xmake build smg-pc-original-fur-shader-tests` and `xmake run smg-pc-original-fur-shader-tests` both exit zero: all five cases pass. The initial native syntax check also passed LLVM 23. `native-tests.json` and `smg-pc-original-fur-shader-tests-final-{build,run}.log` retain exact final validation.

The first runtime pass found an over-strict fixture assertion on the second float displacement: actual buffer identity was correct, the unchanged destination was exactly 88, and repeated deformation produced y = 2^-24 and z = 22 - 2^-19. Diagnostic logs preserve those hexadecimal float values. The second-pass coordinate checks now use the same 1e-5 tolerance already used by the first pass, while both swapped buffer pointers and the unchanged output remain exact checks. No production Shader or math change was made for this failure.

This fixture checks CPU fur deformation and original display-list parsing; it is not evidence of complete Fur rendering, real-disc Mario deformation, or Gateway gameplay.
