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
