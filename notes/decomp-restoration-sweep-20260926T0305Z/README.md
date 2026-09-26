# Restore original code in the port — 2026-09-26

The user requested a codebase-wide sweep to replace port-specific implementations with the existing decompilation. The baseline source-closeness inventory is in `before/`. Its compatibility classification is conservative: formatting, CP932 string conversion, extra compile includes, native ABI corrections, and actual substitute algorithms are grouped together. The count is not the number of behavioral replacements.

## Batch 1: original audio policy and screen implementations

- Restore the original sound emitters, BGM and microphone wrappers, camera audio updates, sound permissions, pause/reset/error audio transitions, and THP audio setup. Remove the disabled-audio wrapper API and duplicate permission flags. BGM now uses the original AudSystem stream manager.
- Restore EncouragePal60Window and GalaxyMapGalaxyPlain from the decompilation. This brings back the original prompt nerve transitions and the original localized GalaxyNamePlate, completion display, and safe-frame positioning. Retain only CP932 literal conversion in those CPP files; their headers match the donor.
- Restore the original number-font tag formatter used by the galaxy name plate. Retain a narrow native-wchar correction: tag lengths count Wii UTF-16 bytes, while the host stores wchar_t elements in four bytes.
- Fix host audio teardown through original JASHeap::free: after the DSP and DVD workers stop, dispose the ARAM root children before the original audio arena is retired. A short process test previously passed its assertions and then crashed in the global ARAM JSUPtrList destructor because the nodes had already been freed.

Validation: main executable and affected test targets build. A 1200-frame real-disc Good Egg process test accepted three original sound effects and verified original stereo streaming and pause/resume; it exited normally. The layout process test checks number-font tags, existing layout transforms/tag lines, and FlyMeter resources. Its 120-frame real-disc run now passes and exits normally after the ARAM cleanup fix. The two newly restored screen classes are compiled and linked, but this test is not an interactive demonstration of their screens. Full logs are adjacent to this note.

No new decompilation was needed. The donor submodule is unchanged. The user-controlled Terrace game was left running throughout; tests use muted output and their own save fixtures.

## Continuing inventory

The remaining-source-candidates report lists source differences for investigation, not planned blind overwrites. Larger substantive candidates include the obsolete Aurora JAudio recipe/archive and AST stream parsers, the Game JMapInfo BCSV facade, native J3D loader glue, and font/resource boundary code. Host byte order, pointer width, ownership, and GPU/audio device boundaries must remain correct when donor implementations replace those paths.

## Batch 2: remove the substitute audio resource and playback pipeline

The original AudNewAudSystem / JAUAudioArcLoader now receives the unchanged SMR resource. The original JAUSoundTable, JAUSoundNameTable and JAUSeqCollection readers consume the big-endian BST, BSTN and BSC structures directly through narrow serialized-field wrappers. The original loader owns copies in the section heap, and the original name converter is created after the original loader publishes its table. Remove the separate converted SMR copy and manually published replacement name table.

Delete Aurora's obsolete BAA/BSC recipe interpreter, bank/wave recipe decoder, AST stream decoder, and PCM voice mixer. No production callers remained for the mixer or its standalone AFC decoder after restoring JAS/DSP playback. Delete the duplicate Aurora JAISoundParams provider; the original src/JSystem/JAudio2/JAISoundParams.cpp is already linked. Update both Xmake and CMake source lists and remove tests of the deleted replacement implementations. This removes 3,436 net lines from Aurora.

Validation: the main executable and original audio test targets build after all deletions. Focused original-reader tests use literal Wii bytes for sound IDs, SE/BGM/stream metadata, name lookup, sequence offsets and bounds, and stream ownership. The real-disc 1200-frame original Good Egg test accepted all three named sound effects, rendered over 1.1 million nonzero DSP samples, passed stereo DVD/ARAM streaming pause/resume and render-independent clock checks, and exited with status 0. Muting affected only the diagnostic output device; the live user game remained untouched.

## Batch 3: original JMapInfo reads original resources

Restore the donor JMapInfo implementation and header, with only serialized byte order, native resource borrowing, and modern C++ scope fixes. The original reader now supplies field hashing, raw string pointers, exact signed/unsigned mask rules, linear/case-insensitive search, and the actual binary search. Delete the decoded DataCompat/string cache, BcsvTable-driven Game getters, float coercion/overrides, and unused synthetic placement/child/rail metadata. Host-owned synthetic resource construction moves to resource/JMapResource; original stage tables retain their archive bytes through one shared borrow, including correct early heap retirement.

Strings now point into the actual resource, so distinct raw aliases no longer share a replacement cached string pointer. Resource headers accept unaligned archive bytes through Aurora's byte-aligned BigEndian scalar wrapper (including signed reads). Existing audio structures that require aligned serialized records keep AlignedBigEndian.

The focused resource and heap-lifetime tests pass. The original Gateway process also resolves and advances RailRider for nine authored placements. Scenario catalog, particle resource, message/talk data, and two-generation scenario publication checks pass. Camera tests exposed old facade-specific expectations (signed getters accepting packed values, getters after end); update them to the donor contract. The effect metadata fixture also had a stale temporary weak lease and double-deleted records already owned by AutoEffectGroup; fix these test lifetime errors before claiming the entire consumer batch passes.

Obsolete synthetic rail-association tests are removed alongside the deleted API. OriginalStageSessionTests now exercises real Gateway lookup and RailRider movement. This is not a replacement claim for the retired synthetic NPC moving-talk assertions; interactive moving-talk coverage remains separate.

Batch 3 final results: all focused JMap reader/lifetime checks, the camera resource suite (including unaligned input and real camera catalogs), and the complete authored effect metadata suite pass. Gateway's nine original rail placements pass lookup and movement. The real scenario catalog covers 48 catalogs and 234 zones; particle checks cover 3,327 names/resources and 2,591 auto-effect rows; message/talk checks and two independent scenario-publication process generations exit normally. All affected original consumer targets compile. Larger legacy actor fixture targets compile; they are not claimed as passing full runtime suites. Logs are adjacent to this note.

## Batch 4: original selection effects and effect-group storage

Restore YesNoController from the donor, bringing back its left/right focus effects, effect cleanup, confirmation sound, and original completion transitions. Restore AutoEffectGroup and AutoEffectGroupHolder's original raw arrays, counts, construction, and lookup implementation, replacing the port-specific vector containers. Keep native destructors for early object/scene-heap retirement; all original behavioral methods are otherwise unchanged apart from one CP932 name literal.

The main executable and affected test targets build. The real SelectButton layout registers both authored cursor effects, and the original selection nerve correctly emits the focused effect and retires the previous one. The fixture commits pending button nerves at their original frame boundary; it does not inject desktop input or claim an interactive click-through. The 120-frame layout process exits normally. The authored effect metadata test checks all 2,591 rows and 612 case-insensitive groups through the restored storage, including fixed-capacity batches, allocation ownership, and clean scene retirement. It also exits normally.

## Batch 5: original NW4R fonts

Restore ResFont, ResFontBase, Font and binary-file validation from the existing NW4R donor. Original code now rebuilds RFNT to RFNU in place, searches direct/table/sparse character maps, follows linked width records, chooses alternate glyphs, computes sheet cells, and reads/writes the actual FINF metrics. Remove the custom 468-line BRFNT decoder plus the ResFont facade and shared per-font cache. The debug font probe also uses the original SDK reader.

Aurora now provides a four-byte relocated resource pointer: original file offsets become field-relative displacements, retaining original record sizes and valid pointers in native 64-bit allocations. Packed numeric fields keep their original big-endian representation. SDK source changes are limited to that relocation boundary, byte-order-aware sparse entries and explicit scalar casts. The existing bounded native SetResource overload checks buffer/block extents before invoking the donor loader. Unlike the deleted facade, original font methods require an installed resource and valid output pointers; tests no longer call those methods with invalid inputs expecting host exceptions.

Validation: main, font probe, font/layout/message test targets build. Literal Wii-byte fixtures exercise all three map encodings, backward width links, alternate selection, shared resource mutations, complete-buffer relocation and independent borrow removal. The actual MiiFont metrics and TextBox binding pass; the real GameSystem PictureFont owner exposes all 50 glyphs and the message corpus resolves all 409 picture tags / 32 payloads. Layout and original message/talk tests pass in 120-frame real-disc processes with normal teardown. This validates font resources and layout consumers, not a new interactive title-to-Good-Egg playthrough. No obsolete BRFNT/cache symbols remain in the linked game.

## Batch 6: original point-light transitions

Restore LightPointCtrl and PointLightInfo from the donor, retaining only native destruction and borrowed actor-generation checks. Remove its private brightness clamp, vector/color interpolation, and zero-duration lighting workaround. It now calls the original shared MR math helpers. Original zero-duration easing is supported by routing trigonometric table index conversion through Aurora's existing Gekko integer-conversion helper; color interpolation uses the same helper before the original low-byte store. This also defines NaN, overflow and extrapolation consistently for other callers. Dolphin's Interpreter_FloatingPoint.cpp confirms integer-indefinite for NaN/negative overflow and signed maximum for positive overflow.

The main executable and light test build. The 120-frame real-scene light test passes authored light catalogs, world/camera-space blending, nearest-candidate selection, NaN brightness, expired/reused actor addresses, fade-in/out endpoints and midpoint, zero duration, and original GX light commands. Additional checks verify NaN trigonometric phase and exact color byte results for extrapolation/NaN/positive infinity. All runs exit normally.

## Batch 7: one canonical provider per original function

The linked-provider audit found 11 duplicate strong definitions: four gravity helpers, four attribute-group helpers and three collision-grid helpers. Delete the port-only LiveActorUtilGravity and LiveActorUtilGroup units; the original functions already live in LiveActorUtil. Remove the duplicate collision bodies from KCollision and use the donor's KCollisionPlus unit, carrying over only the general Gekko float-to-integer conversion fix. The final configured-archive audit reports **zero duplicate strong definitions**, down from 16 at the beginning of the sweep and 11 before this batch.

Validation: the main executable, collision resource, Gateway session and group tests build. All 12 collision resource cases pass. Gateway initializes and runs 120 frames, including original rail lookup/movement for nine placements and normal retirement. Group membership, decomp attribute-group helpers and 32 scene lifetimes pass. The old group fixture required repair before it could run: it omitted the original clipping director and did not select the actual scene allocation heap. It now constructs those real owners with sufficient fixture storage. Remove facade-specific invalid-argument assertions from the legacy NPC fixture; the original helper's caller must supply its scene manager and actor. No full legacy NPC runtime pass is claimed here.

## Batch 8: original message tag replacement and skipping

Restore ReplaceTagProcessor and MessageTagSkipTagProcessor from the donor, including their original headers, local dispatch tables, parameter API and page-boundary behavior. Remove the port-only MessageEditorMessageTag header and duplicate out-of-line group predicate; the original class belongs to MessageTagSkipTagProcessor. Retain only the native wide-character stride/byte-order accessors, picture-header emission and portable va_copy/va_end handling. Existing consumers include the original owner header directly.

Validation: the main executable and both original layout/message process targets build. The 120-frame message process passes original font/tag dispatch, all loaded message/talk data, number padding with indexed varargs, native string-pointer varargs, player-picture substitution, page cutoff, preserved unknown tags with embedded zero words and byte/word payload addressing. The 120-frame layout process also passes its original layout, FlyMeter and selection-effect checks. Both processes exit normally with device output muted.

## Batch 9: original dialogue drawing and reveal

Restore CustomTagProcessor and TalkTextFormer from the donor, including the specialized original NW4R wide-text PrintImpl. This removes the fragmented port renderer/reveal rewrite and restores original field names, dispatch, text sound mask, colour selection, word wrapping, picture/number glyph paths, ruby placement and Korean suffix logic. Keep only the existing native argument-pointer, packed UTF16/ruby and Gekko conversion fixes, plus required includes and omission of unavailable donor debug-assert macros. Both class headers now match the donor. LayoutCoreUtil uses the original mIsInfo field name.

Validation: the main executable, original layout test and original message/talk test build. Both 120-frame real-resource processes pass and exit normally. New layout assertions exercise repeated signed numeric arguments, full-width borrowed string pointers, null substitutions, original half-alpha truncation, authored waits during drawing versus measurement, and end-delay saturation. Existing real FlyMeter rendering and SelectButton effect transitions continue to pass. The message test validates 957 talk messages, 542 branches, 92 events, 291 terminals and the actual message-holder resources. Japanese ruby handling retains its established native-byte adapter; this Korean-disc run is not a Japanese dialogue playthrough.

## Batch 10: original model joint factory

Link the actual donor J3DJointFactory and remove J3dJointData's direct construction of joint state. The host adapter now decodes bounded JNT1 initializers/remap indices into native metadata; the original factory performs logical numbering, remapping, transform/bounds assignment and scale-compensation sentinel handling. The factory CPP is identical to the donor. Its initializer's scale-compensation field is a byte on the host: authored values include 2 and 0xff, which cannot be represented by a C++ bool. Retained original joint objects replace the facade's vector of manually populated objects.

Validation: main, joint-resource, complete-model and Gateway session targets build. All four joint-resource groups pass, including the real Mario model (30 joints, 13 envelopes, 35 draw matrices, 22 full-weight matrices). All four complete-model groups pass across original BDL material modes and BMD finalization. Gateway runs 120 frames, verifies nine original rails and exits normally. The rest of the native model loader remains a separate restoration task; this batch restores the real joint factory, not the entire loader.

## Batch 11: original animation blending and material flags

Restore XanimeCore and MaterialAnmBuffer from the donor. XanimeCore's blending, quaternion interpolation, freeze/update phases, scale compensation and Basic/Maya/Softimage paths now use the original bodies. Keep the existing native shared lifetime for joint/transform arrays and separate track ownership; keep the optional local-matrix field at native pointer width. Remove scalar/union aliases in favour of the donor's vector and transform fields. Restore the explicit J3DTransformInfo padding field used by the original constructor without changing its record size. MaterialAnmBuffer CPP/header exactly match the donor.

Validation: main and affected test targets build. All nine XanimeCore system groups, six XanimePlayer lifecycle groups and five material-animation groups pass. Test fixtures use the restored field names and explicit padding initializer. Gateway then runs 120 frames, verifies original rails and exits normally. The refreshed configured-provider audit still reports zero duplicate strong definitions.

## Batch 12: canonical movie renderer source

Remove the duplicate THPDraw.cpp and compile the existing THPDraw.c as C++. Both copies were identical to the original donor C file; the retained source is unchanged. This removes a misleading port-only audit entry without changing movie rendering behavior. The other unmatched file, GalaxyIDBCSV.cpp, contains an embedded byte-array resource whose original symbol is recorded in the donor; it is not replacement game logic.

Validation: the main executable builds and links the canonical THP source. The refreshed audit reports zero duplicate strong definitions. No new runtime claim is needed for this source-list-only change.

## Batch 13: original save chunk serialization and error handling

BinaryDataChunkHolder.cpp now exactly matches the donor. Its serialized header uses byte-aligned big-endian fields and an actual twelve-byte payload boundary, so the original code works on unaligned chunk records and native 64-bit hosts. Remove the replacement two-pass loader and extra validation virtual from BinaryDataChunkBase. Restore SysConfigFile's original handling of the loader return value. The concrete chunk deserializers retain their existing bounded payload checks; moving those remaining schema checks is separate work.

Delete the unused common chunk encoder, decoder, copied payload representation and scalar/hash helpers. The host NAND boundary now uses a small, non-allocating extent check before publishing files to the original unsized reader. It checks actual member ranges and rejects overflowing outer sizes before checksum access. Hash policy stays in the original code: matching chunks are deserialized even after a hash mismatch, unknown signatures are skipped, and deserialization errors are reported after later chunks are processed.

Validation: main and four save-related test targets build. Six focused config groups and the original player-status storage suite pass, including Dolphin golden bytes, unaligned thirteen-byte chunks, hash/error continuation, all truncated extents, and a malformed inner chunk with a recomputed valid outer checksum. Tests modify only in-memory copies of the oracle. Both real Gateway process tests complete 120 frames and exit normally: the save owner covers six chunks, 188 flag lookups and 14 story thresholds; the star owner covers 42 galaxy records, 121 stars, 18 hidden stars and seven grand stars. Former assertions about port-specific atomic loading now check the original partial-application behavior. The provider audit still reports zero duplicate strong definitions.

## Batch 14: original collision categories and zones

Restore CollisionCategorizedKeeper and CollisionZone from the donor, including their original fixed-array vectors, field names, spatial bounds, point/sphere/line queries, polygon-area collection and membership removal. Keep the existing native geometry-publication checks and destruction of owned zones/hit records, plus CP932 literal conversion. Restore the missing original MR::Vector::pop_back helper instead of rewriting its collision caller. The source now differs from the donor by 17 added lines and one CP932 literal replacement.

Validation: main and five affected test targets build. The focused map-query suite passes sphere features/translation/thickness, ordering/filtering/moving reactions, point boundaries/scale/output, segmented lines/exclusion/enclosure, polygon areas and retirement. The collision ownership process runs 120 Gateway frames and checks original moving transforms, sensor ownership, shadow projection and teardown. Two 360-frame real-disc processes pass: CollisionArea checks both authored placements and twelve face mutations; MapObj checks eight authored owners, three Low models, eleven collision parts and eleven active face queries. Those map objects were clipped during subsequent observations, so that run does not establish visible rotation or gameplay parity. All three current process runs exit normally.

Test fixtures needed updates to the original vector fields and the current scene/resource ownership APIs. The first compilation also exposed the missing pop_back helper. Two diagnostics were inadvertently launched before that failed build was noticed; their output is excluded from validation. The separately named current query/owner logs and the area/object logs were captured only after the full target build succeeded. No user save or controls were used.

## Remaining work

This is an ongoing restoration sweep, not a claim that every source difference has been eliminated. The current audit covers 3,211 Game files: 1,715 exact-source, 375 compile-only, 1,120 compatibility-temporary and one embedded-data file without a donor source counterpart. The compatibility-temporary category includes native ownership, serialized byte order, wide-character/varargs ABI work, formatting, and true replacement logic. Larger substantive work remains in the complete J3D model loader, archive/resource adapters, save-data validation boundaries and other construction facades. The 6,161 unreviewed symbol providers are an inventory, not a count of replacement algorithms. The summary in after/ records this checkpoint; raw audit tables remain local. No interactive full-game playthrough was performed in this sweep, and the user-controlled save and game inputs were untouched.


## Batch 15: original J3D model-loader material readers

Restore the donor J3DModelLoader readers and calculator implementation, and call the original material readers from the retained resource boundary. Remove the host material-construction algorithms. Preserve native bounds checks, decoded data, allocation retirement and original-width identity addresses. The original full-file loader still needs a native file representation before it can replace the remaining joint/geometry orchestration; do not claim that work complete.

Implemented and validated:

- `J3DModelLoader.cpp` now contains the donor implementation. The active resource boundary calls its original v26 normal/table/patched/locked material readers and `modifyMaterial`; the copied host construction loops were deleted. Finalization calls the original `setupBBoardInfo` instead of its hand-copied facade.
- Donor `J3DModelLoaderCalcSize.cpp`, `J3DMaterialFactory_v21.cpp` and its header are compiled to provide the actual SDK classes/vtables. These three files are byte-identical to the donor. This does not yet add native MAT2 decoding or claim full-file original loader dispatch.
- Remaining changes in the SDK loader are the existing bounded-resource entrypoints/finalization bridge, public GX typedef spellings, omission of a redundant implicit-constructor definition, and the original-width address conversion at material identity casts. Scoped bindings reject unrelated allocations sharing one identity and restore enclosing bindings during nested loads.
- Native material ownership captures the actual arrays/materials published by the original readers. Byte decoding, authored range validation and retained-heap teardown stay outside Game. No Game algorithm changed.

Evidence:

- Current `smg-pc` and three validation targets built successfully (`batch15-build.log`). The initial compile needed the public GX typedef names; no tests from that failed build were used.
- `batch15-model.log`: four complete-model groups pass with real Mario BDL data, all three binary material modes, BMD unique-material links/Wii stride identities, nonempty BMT unshifted identities, resource alias lifetimes, malformed-boundary rejection and complete heap retirement. Added scoped-identity checks cover nested restoration and invalid aliasing.
- `batch15-material.log`: four material-resource groups pass, including all real Mario normal/patched/locked factories.
- `batch15-stage.log`: real Gateway scenario 1 ran 120 frames, checked 9 original RailRider placements, and exited through normal teardown. Audio was muted; this is bounded integration evidence, not a fresh whole-game playthrough.
- Audit: zero duplicate strong providers; 6,237 unreviewed provider records. Game file classifications remain 1,715 exact, 375 compile-only, 1,120 compatibility-bearing and one embedded-data-only file.


## Batch 16: original J3D joint and matrix readers

Replace the host INF1/JNT1/EVP1/DRW1 construction and field publication with calls to the original loader readers, keeping endian decoding, source bounds and allocation ownership in the resource component.

Implemented and validated:

- The joint resource component now prepares retained native INF1/JNT1/EVP1/DRW1 blocks. `J3DModelLoader::readInformation`, `readJoint`, `readEnvelop` and `readDraw` populate the actual destination model. The host calculator-selection switch, joint creation loop and manual assignment of the model's joint/matrix fields were deleted.
- Original draw/envelope block order is preserved, including DRW1 before EVP1. Original `readDraw` now performs the envelope subtraction, leading flag scan and important-matrix allocation. Independent input checks still reject unsafe source ranges and cross-table references before construction.
- Original-created joints, name/pointer arrays, calculator and important-matrix storage have explicit native owners. Full-model attachment enters the retained SDK heap for original allocations; the host ownership vector remains outside that heap.
- `batch16-build.log`: current main application plus joint-resource, complete-model and Gateway targets build successfully.
- `batch16-joint.log`: four groups pass. Coverage includes all three matrix calculators, remapped initializer values, absent and short names, source retirement/moves, malformed input, original block-order behavior and real Mario counts (30 joints, 13 envelopes, 48 serialized draw entries, 35 effective draw entries, 22 full-weight entries).
- `batch16-model.log`: four complete-model groups pass, including new actual heap-provenance checks for the original joint-reader allocations and normal resource/root-heap retirement.
- `batch16-stage.log`: real Gateway scenario 1 completes 120 frames, nine RailRider checks and normal teardown, with audio muted.
- `batch16-audit.log`: zero duplicate strong providers; Game source classifications unchanged.

Remaining model-loader restoration: VTX1 count arithmetic must preserve authored offsets across widened/repacked native arrays; SHP1 ownership must capture every allocation when INF recreates a logical shape slot; full-file loader dispatch must then replace the remaining host orchestration. These boundaries remain explicit work, not a claim that all port implementations have been removed.
