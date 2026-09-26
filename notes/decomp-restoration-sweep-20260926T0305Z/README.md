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
