# J3D material owner restoration — 2026-09-25

This bounded batch removes nine compat providers by restoring six complete original JSystem translation units. It preserves the native endian, pointer and texture-matrix floating-point adaptations, and restores the original light-update dispatch that the split compat implementation had lost.

## Source ownership

| Canonical owner | Removed providers / fragments |
| --- | --- |
| J3DGraphBase/J3DMaterial.cpp | MaterialFactoryCompat, MaterialHelpersCompat material methods, MaterialVariantsCompat |
| J3DGraphBase/J3DMatBlock.cpp | MatBlockCompat |
| J3DGraphBase/J3DTevs.cpp | TevsCompat, TexMtxCompat, MaterialHelpersCompat loadNBTScale, J3DSysCompat texture/alpha/depth/swap table builders and storage |
| J3DGraphAnimator/J3DMaterialAnm.cpp | MaterialAnmCompat |
| J3DGraphAnimator/J3DMaterialAttach.cpp | MaterialAttachCompat plus MaterialTable clear/constructor/destructor from J3DModelDataCompat |
| J3DGraphAnimator/J3DShapeTable.cpp | ShapeTableCompat |

The remaining J3DSysCompat and J3DModelDataCompat code retains its current implementation; only the methods whose complete original owners are restored here were removed. J3DSysCompat includes the original Tevs declarations to call its moved table builders. No Game, scene, demo, renderer, Aurora or build configuration files were changed.

## Original behavior restored

- J3DColorBlockLightOn::diff dispatches to diffLight for either the color-channel flag or any light-count bit (bits 4–7). Its original diffLight writes channels and loads every non-null light in its actual hardware slot. The old fragment omitted all lights and ignored the light-count mask.
- J3DColorBlockLightOff and LightOn now override the actual base diffLight virtual slot. The old declarations introduced separate diffColorChan virtuals while inheriting the empty base diffLight; even LightOff's original diff dispatch could consequently do nothing.
- TevBlock1/2/4 resets use the current donor's TevOrderInfo assignment; the original explicit J3DTevOrder copy operator is restored with its declaration.
- loadTexNo retains the donor's explicit signed s8 interpretation for minimum/maximum LOD. This is source-checked rather than a claimed runtime LOD correction: the downstream GD encoder's float-to-u8 conversion needs its own PPC conversion audit for negative/out-of-range inputs.

## Native boundaries preserved

- Packed RGBA and unaligned BP texture-register reads use Aurora big-endian reads. Direct Wii u32 reinterpretation would reverse bytes or require alignment on a native host.
- Texture-matrix calculations keep the preexisting Clang floating-point contraction-off directive. Original matrix algorithms, GX upload calls and native ResTIMG relative addressing are retained.
- Material allocation budgets use the donor's sizeof(native type), unchanged. Existing J3DPacket device/interrupt/native recording scopes are untouched.
- The existing j3dDefaultLightInfo value is retained in Tevs.cpp. Current decomp declares the default but does not define it; deleting the old provider must not lose its definition.
- Small displaced original helpers live with their canonical declarations: J3DGDSetZCompLoc in J3DGD.hpp; loadTexCoordScale in J3DTevs.hpp; GXColorS10 and TexCoord assignment bodies beside their existing native class definitions. The duplicate inline GD command-header definition from MatBlockCompat was removed; J3DGDCompat remains its single provider.

## Regression and checks

OriginalJ3DMaterialBlockTests.cpp adds a real GD command-buffer regression. It exercises the two light-count bits and color-channel flag through the base virtual interface, verifies sparse light slots 0 and 7, the exact XF register sequence, big-endian float/color values, command extent and untouched guard bytes. It also checks direct diffLight virtual dispatch, material-color-only updates, and LightOff behavior.

`python3 notes/compat-owner-removal-round2-20260925/j3d/validate-source.py` passes 27 checks. All 228 detected original functions are present once in source. The only body differences from current donor are the two endian color reads, endian texture-register read and SDK enum spelling in two texture-matrix upload helpers. Four owners are copied exactly apart from ShapeTable's necessary complete-type include. The scanner is lexical, not a compilation or runtime proof.

All 16 existing touched files were clean when this batch began; `before.json` records status/hash and `baseline/` preserves copies. `source-changes.patch` contains the exact batch diff. Added-line whitespace checks and `git diff --check` passed. No preexisting dirty files were edited.

**No build, runtime test, Git index, staging or commit operation was performed in this lane.** Parent owns full native build and test execution. Add the six canonical paths listed in `build-wiring.json` to the Game native source list; the nine deleted providers leave the existing compat wildcard automatically. No new test target is needed. Existing material-block, material-table, texture-mtx, material-resource and geometry-resource targets cover this batch.

The broader shape draw/matrix/factory, material resource loader, generic animation, J3DSys native serialization and J3DModelData ownership systems remain separate follow-up batches. This checkpoint does not claim Gateway gameplay or visual completion.
