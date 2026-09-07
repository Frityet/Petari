# Original matrix scale extraction

The actual CollisionParts owner exposed a missing native `TRotation3<TMatrix34<SMatrix34C<f32>>>::getScale` specialization. Recovered the retail 172-byte symbol at 0x80175D4C in its reference owning CollisionParts TU, then copied the specialization body into the existing native SDK TMatrix.cpp. No actor-specific behavior is involved.

Each output is the magnitude of one matrix column. Translation is ignored; mirrored and nonorthogonal bases retain unsigned column magnitudes. The arithmetic squares all three components separately, then sums `z*z + (x*x + y*y)` and calls the existing JGeometry TUtil sqrt. Parent was asked to compile native TMatrix.cpp with `-ffp-contract=off`, preserving the retail separate multiplies and additions.

Fresh full reference CollisionParts compilation passed; this symbol scores 97.06977%. Side-by-side disassembly has the same size, arithmetic, stores and three sqrt relocations. The remaining differences are component load/multiply scheduling. The copied native body is byte-identical to the reference specialization. Native O0 and O2 isolated compilation/link/run pass identity, off-diagonal scaled columns, negative components, translation independence, degenerate zero columns and nonorthogonal columns. Full original collision-owner fixture remains parent-coordinated; this does not claim that runtime result.

The native existing TUtil sqrt implementation uses the platform square root, while retail uses its reciprocal-square-root estimate refinement. This recovery preserves the existing SDK boundary and is not a new claim of bit-identical sqrt across exceptional inputs.

Evidence: `retail.asm`, `candidate.asm`, `CollisionParts-compile-command.json`, `native-proof.json`, and `ScaleTests.cpp`. Binary objects/executables need not be committed.
