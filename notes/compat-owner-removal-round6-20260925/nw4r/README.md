# Canonical NW4R and CPU synchronization owners

Remove Nw4rDiagnostics.cpp, Nw4rFontCompat.cpp, Nw4rLayoutRecordsCompat.cpp and PPCArchCompat.cpp from compat.

Font's native constructor, destructor and retained-resource observer join the original character-reader implementation in ut_Font.cpp. The complete existing ResFont implementation and private bounded BRFNT helpers now live in ut_ResFont.cpp. Their implementation is unchanged; the native decoded representation still backs all public metrics, glyphs, encoding, resource installation/removal and generation tracking. This consolidates the current native font interface; it does not claim the original ResFontBase hierarchy or its in-place RFNU pointer format has been restored.

lyt_pane.cpp starts from the full current donor file. It retains the necessary default native Pane constructor and the five existing binding hooks: rename and two hierarchy mutation checks, native matrix synchronization, and native animation synchronization. All 35 donor function bodies remain otherwise unchanged, apart from removing a top-level const on the returned VEC2 to match the existing declaration. Original child/material ownership, animation-link traversal, visibility, transforms, alpha propagation, draw traversal, colour defaults and matrix loading now appear together in their canonical SDK owner. The existing typed layout resource binding is unchanged; replacing that broader native renderer is outside this batch.

The native Panic implementation belongs to NW4R db_assert.cpp and uses Aurora's allocation scope directly. The old JKR name was an alias for that exact scope. The native exception behavior is preserved; Wii framebuffer stack-trace and halt behavior is not newly implemented.

PPCSync now belongs to Aurora's OS library at lib/dolphin/PPCArch.cpp. Its sequentially consistent CPU fence is unchanged. Both Xmake and CMake include it; no alias, forwarding provider or Game dependency is introduced. This change does not implement other currently absent PPCArch APIs.

The first integrated compile exposed a missing original MTX34Copy helper and an ambiguous MTX34-to-void-pointer conversion. The helper is restored exactly from the donor math header. Pane explicitly supplies the same matrix array to GXLoadPosMtxImm. These are SDK compilation adaptations. The initial compiler log is retained.

verify_source.py checks complete source preservation and donor pane bodies independently of the eventual build. It passes 50 checks, allowing the documented pointer disambiguation. Runtime/build results are recorded by the root round6 validation, separately from this source comparison.

The existing Mii font regression previously skipped its retail half when extracted archives were absent. It now reads MiiFont.arc and FileInfo.arc from the supplied disc inside the actual original process, stores only temporary fixture copies, and runs every existing resource/metrics/glyph/rebinding assertion. The temporary copies are removed on success or failure. This does not add a second Game language or runtime owner.
