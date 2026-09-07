# Original shared player providers, 2026-09-07

This closes the assigned general helpers exposed by the complete original Mario
constructor/state graph. No actor-specific branch or placeholder was added.

- The entire original `DirectDrawUtil.cpp` is copied byte-for-byte into native
  Game: six functions, including format/TEV/lighting setup and both vertex
  overloads. The ordinary Game source glob activates it.
- `getTexture` and `onCalcAnim` are imported into existing original access
  providers. The latter clears the original animation flag directly.
- New `OriginalMtxGeometry.cpp` contains complete original makeMtxUpSide,
  makeMtxFrontUp and rotAxisVecRad bodies. It is picked up by the compat glob.
- calcSpherePos/getSphereRadius are newly recovered in decomp, then copied into
  native AreaObjUtil. Retail reads AreaObj.mForm at 0xC, tail-calls
  AreaFormSphere::calcPos, and reads the sphere radius at 0x14. Both are 100%
  matches (16 and 12 bytes); `area-sphere-retail.asm` records the evidence.

The old ModelUtil reference declared getTexture as JUTTexture*. Its actual
ResourceHolder table stores raw BTI bytes, and original MarioFoo immediately
passes the return to the JUTTexture(ResTIMG*) constructor. The reference return
and cast were corrected to ResTIMG* first, then mirrored into native declarations
and provider. The 8-byte Wii body remains 100%; both native provider and original
MarioFoo compile after the type correction. No runtime resource translation was
inserted at the accessor.

Fresh whole Wii translation-unit compilation passes for AreaObjUtil,
DirectDrawUtil, MtxUtil, ModelUtil and LiveActorUtil. All requested helpers score
100% except the two basis routines (99.48% / 99.81%). The additional three-vector
DirectDraw overload scores 88.17% because the current SDK inlines GXNormal3f32;
the original and recovered disassembly both write the same three normal floats
between position and optional UV (`direct-draw-three-vector-compare.asm`). Its
other five functions score 100%. Full symbol results and commands are retained.

All five native source TUs pass isolated syntax. The existing math target builds
and runs 0 with new checks for the original up/side orientation, front/up
normalization, untouched translation, argument order and both destination
aliases of axis rotation. New `smg-pc-original-direct-draw-util-tests` builds and
runs 0 using real Aurora GX display-list recording: six formats, eight vertex
overload paths, real descriptor/format state, and exact big-endian float bytes.
No GPU is started by that FIFO test.

The fresh showcase compiles and reaches 27 unresolved original owner/provider
symbols, down from the prior 46 after this and the parent's additional original
owner imports. All assigned helpers resolve. `showcase-undefined.txt` retains
the exact remaining origin references. This is source/link progress; it does
not claim a new playable Mario runtime.

`root-checkpoint-paths.json` and `decomp-checkpoint-paths.json` specify exact
commit cohorts. Parent owns both indexes; no root/decomp staging was performed.
