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
