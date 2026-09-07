# Original water screen owner recovery

This records original source recovery and the newly activated native screen/water owner graph. All 27 affected native TUs pass isolated syntax; parent reports the combined production archive build passed. Focused native ownership/query tests are running under parent coordination. No rendered water/Bloom or gameplay success is claimed. Stable collision/getScale work has a separate manifest under original-collision-scale-20260907 and original-collision-owner-20260907.

Twelve recovered or corrected original functions compile with the Wii compiler:

| Function | Retail bytes | Fuzzy match |
| --- | ---: | ---: |
| BloomEffect::postDraw | 956 | 99.83% |
| BloomEffect::drawTexture | 660 | 99.64% |
| BloomEffect::initBlurMtx | 560 | 94.39% |
| BloomEffect::drawBlur | 644 | 99.63% |
| ImageEffectLocalUtil::setTextureTrans | 100 | 100% |
| ImageEffectLocalUtil::capture | 132 | 100% |
| ImageEffectLocalUtil::blurTexture | 540 | 94.84% |
| StateBloomNormal::update | 652 | 99.54% |
| ImageEffectAreaMgr::sort | 172 | 100% |
| anonymous getFirstPolyOnLineCategory | 344 | 98.66% |
| getFirstPolyOnLineToWaterSurface, two wrappers | 16 / 20 | 100% |

The four Bloom functions were missing. They retain the exact capture/compositing order, tile coordinates, GX matrix batches, per-pass intensities and six-/twelve-point circular blur kernels. postDraw disables and restores the copy filter at the retail boundaries. Its last two compositing passes occur after restoration. initBlurMtx and the shared blur use JMath trigonometric tables and original float expression order; remaining differences are compiler register/spill scheduling, not substituted math.

ImageEffectLocalUtil::capture previously divided by the tile index, including zero. Retail instead uses signed integer tile row/column multiplied by the corresponding framebuffer dimension, divided by the grid size. Its original setTextureTrans uses an affine translation matrix and GX_MTX2x4; the previous dormant code had an extra nonzero matrix term and the wrong matrix type. blurTexture was absent; it now preserves aspect-scaled vertical radius, u8 per-pass intensity, first-pass overwrite, subsequent additive blend, and final matrix reset.

StateBloomNormal::update previously discarded two expressions where retail stores the normalized blur intensities. Retail converts each smoothed float to u8 before writing intensity/255.0 to Bloom. The original mIntensity1 and mIntensity2 fields are signed 32-bit, proven by the xoris-based signed conversions. Restoring those types and stores produces a 99.54% function match. Header field offsets and sizes are unchanged.

Fresh full Wii compilations of all twelve candidate screen/holder TUs pass. The original preliminary object inventory used native headers first and decomp fallback for missing headers; it was not a link proof. The final `native-integration-syntax.json` checks 27 actual production TUs against native headers without fallback, all exit 0. The direct source imports use unchanged reference bodies except native GX include spellings/PPCSync declaration and the original bool water return declarations. `native-paths.json` enumerates the final cohort. Shared factory/header paths also contain parent and other agent changes; the parent checkpoint owns their final combined hashes.

The six reference paths in `decomp-paths.json` and `decomp-hashes.json` include the new sort and water line query. `recovered-functions.json`, focused retail/candidate assembly, and per-TU compiler manifests record proof. The temporary recovery scripts and object/large objdiff dumps are not required for the source checkpoint and must not be rerun over the final recovered files. The existing MR u8/GXColor lerp providers were separately rebuilt and each matches 100%; the unusual retail green component interpolation toward end.b is preserved.

## Native ownership and rollback

The normal Game archive now contains original WaterAreaHolder/WaterCameraFilter, ImageEffectSystemHolder/Resource/Director/State/Base/LocalUtil, BloomEffect, BloomEffectSimple, ScreenBlurEffect, DepthOfFieldBlur and CopyFilterNegater. Original Water/Bloom/SimpleBloom/ScreenBlur/DepthOfField areas register through exact managers with retail capacity/order and cube origin conventions. ImageEffectAreaMgr::sort is the exact unstable selection sort on signed mObjArg7 (100% match). No Water/WhirlPool actor-specific branch or fake query was added; WhirlPoolAccelerator has only its completed reference constructor/query TU and remains absent from the actor factory because its full behavior is not recovered.

`ImageEffectOwnership` calls the original six SceneObj constructors under the actual scene Game domain. SceneObjHolder continues to own original registered NameObjs, including Bloom draw adaptors, ImageEffectDirector and WaterCameraFilter. The compatibility owner snapshots completed raw state/matrix/array children, captures SDK textures, and reclaims them after NameObjs on scene retirement or failed factory rollback. It requires an actual Game domain so incomplete original constructor raw allocations remain in the original scene arena. Successful roots are fully reclaimed; a constructor failing before returning can leave plain arena bytes until that domain retires, consistent with the existing original scene allocation boundary, but completed SDK textures are immediately reclaimed.

The explicit `JutTextureConstructionScope` tracks only audited heap-constructed textures within the selected call boundary. Its stable live records are invalidated by the actual SDK destructor, including heap-finalizer destruction, so an address reused for a new texture is never mistaken for the earlier construction. Nested successful adoption and disabled capture preserve independent owners. Adoption uses list splice and does not allocate during exception unwinding. There is no global finalizer-age rollback across resource heaps.

Original ImageEffectResource publishes textures as each allocation succeeds. Capture transfers those exact field identities to the resource owner even if the requesting Bloom constructor subsequently fails. If the resource holder predates a failed outer factory it remains valid for retry; if it belongs to the rolled-back registration suffix, its textures retire with it. JUT borrowed construction now includes storeTIMG within the cleanup handler; owned construction retains its existing allocation RAII, and failed completion registration also unregisters its heap finalizer before unwinding.

The two former native ScreenUtil facade calls now invoke their exact original ImageEffectDirector bodies. Additional exact original MR creation/control/color/fullscreen-fill wrappers live outside Game in OriginalImageEffectUtil.cpp. Actual scene bootstrap still must create required original owners before clients query them; absent owners are not replaced by false/null answers.

## Water collision query

The native WaterSurface overloads use the real category-2 service owned by CollisionDirectorOwnership. They retain original registry/prism encounter order, parts filtering before the 32-hit capacity, triangle filtering after capacity, strict nearest selection with a 1,000,000.0 initial cutoff, stable tie selection and independently optional position/triangle outputs. Filters run with the caller allocation state restored. Existing map-only nearest queries were not rewritten by this cohort. Production dynamic water entries retain actual CollisionParts identity; synthetic geometry-only triangle metadata still assumes the existing map service and is not claimed as auxiliary metadata support.

The existing line-query fixture now covers absent ownership, map-vs-water categories, ordering/ties, optional/unchanged outputs, all 32 rejected triangles excluding a later hit, pre-capacity sensor exclusion and caller Game allocation restoration. Syntax passes; parent owns the runtime result.

## Validation boundaries

Aurora now implements retail GXLoadTexMtxIndx: GX_LOAD_INDX_C with actual texture-array index/stride, 8/12-word length and normal/post XF destination. The release retail API permits 2x4 postmatrix loads; the decoder preserves the untouched third row. All 258 independent Aurora FIFO/display-list tests pass, including two new endian/stride/partial-row tests. This is CPU decoding proof, not rendered Bloom proof.

Existing JUT ownership tests were extended for cross-heap rollback, nested/partial shared adoption, disabled capture, heap destruction before capture ownership, exact address reuse and failed owned construction. `jut-construction-syntax.json` records all 3 affected units passing syntax. New OriginalImageEffectOwnershipTests uses an actual RVZ filter archive and tests original owner/control transitions, shared resource survival during nested factory failure, same-binding retry and two scene teardown cycles. Its final syntax result is `owner-test-syntax.json`; parent is running the target. It does not exercise the full draw/composite pipeline or demonstrate swimming/jumping/gameplay. Parent owns final test logs and root link inventory.

## Final link leaf closure

The shared production archive compiled, and the extended JUT ownership target built and ran successfully under parent coordination. The next strict link required four existing original water leaves. Whole original OceanBowlPoint and WaterPoint TUs/headers are now imported; their height queries freshly match 100% and 99.5098%. The exact current/next rail-point Arg0 float wrappers are added beside the existing Arg1 wrappers in GameRailCompat; both freshly match 100%. All three native units pass syntax. These are existing source imports, so no new decomp changes were made. `existing-leaf-proof.json` and focused assembly/compiler manifests record those proofs. Parent owns the next strict link result.

Review also corrected the new descriptor table insertion to preserve increasing original manager order (Water 10, ImageEffect 39). The image owner fixture now calls the actual Director movement step after requesting normal Bloom and before the effect calcAnim; requesting a state alone does not perform its update. Both corrected files pass syntax.

The obsolete OriginalImageEffectOwnerQuery compatibility TU is deleted. Its seven owner/subjective-control definitions are now supplied by the full original ImageEffectDirector/SystemHolder and exact OriginalImageEffectUtil wrappers. This removes duplicate symbol ownership instead of relying on static-archive member selection. A null hash in native-hashes.json records this explicit deletion.
