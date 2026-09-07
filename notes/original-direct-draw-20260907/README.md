# Original direct drawing and texture access — 2026-09-07

Restored the retained DirectDraw translation unit from root preflatten commit
e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4, then compared it against the extracted
RMGK01 object using the original Metrowerks compiler. The source was recovered
and repaired in decomp before mirroring it into the native Game module. Native
activation replaces the former partial OriginalDirectDraw.cpp and duplicate
fog/color wrappers with the full original source file.

The retail comparison found and corrected real retained reconstruction defects:

- drawSpherePart now divides loop indices as floating point, uses the original
  interpolation parentheses, increments the actual angle, and uses the unsigned
  inner loop index. Its score is97.89474%.
- drawSphere3D passes color and segment counts in the original order and uses
  the original0..2pi and0..pi limits.
- setTexel32 restores the actual RGBA8 alpha/red and green/blue planes. The
  retained source had green and blue reversed.

Newly recovered functions are the two project2D overloads (99.62687% and100%),
both getTexel32 overloads, and the JUTTexture setTexel32 overload. Both JUTTexture
wrappers score100%. Project2D uses GXProject with the actual camera matrices,
viewport, projection coefficients, and original widescreen scaling. The missing
standard GXProject declaration was added to the decomp GX SDK header.

wii-compile.json records the exact compiler argument array, final source hash,
and every function score. Low aggregate/symbol scores are not treated as proof
of correctness. Raw texel functions differ in arithmetic grouping/register and
address-update selection; inspection verifies the four byte offsets and channel
shifts against retail. The native regression uses an independent sequential tile
encoder across six4x4 tiles and guard bytes, rather than a getter/writer roundtrip.
Its execution result is recorded separately after the native integration run.

The lower retained drawFillBox3D score (66.251205%) was checked against the
original eight corner constructions and all six face vertex orders. The source
preserves those operations; vector constructor/assignment inlining and GX output
inlining differ. drawSphere/drawSphere3D likewise differ in vector/matrix inline
selection. No SDK vector/matrix changes were made just to raise scores.

This checkpoint does not claim complete DirectDraw coverage: the tileConversion
functions are still absent. Native application rendering and Gateway gameplay
remain separate validation work.

Also recovered MR::calcCylinderCenterPos as the original16-byte delegate to
AreaFormCylinder::calcCenterPos; objdiff100%. See area-center-proof.json.
