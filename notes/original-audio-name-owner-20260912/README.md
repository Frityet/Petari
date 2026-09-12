# Original sound-name owner and disabled audio closure

The full original SoundUtil now calls the actual AudSoundNameConverter. Closing only its missing lookup symbol would dereference an unpublished singleton. This change imports the complete existing reference converter and SDK JAUSoundNameTable, validates the real BSTN archive, converts only its fixed-width numeric words to native endian, and publishes the complete original table/converter for the runtime lifetime.

The existing DisabledObjectAudioService retains the actual root heap and a child JKRSolidHeap sized from the original constructor's object and two arrays. Names borrow the separately retained native BSTN byte buffer. Nested service retirement restores the previous original table and converter. A constructor failure restores the previous table and releases the provisional heap. No partial AudSystem, rhythm system, or speaker system is fabricated.

Original AudMeNameConverter and CSSoundNameConverter complete translation units are imported unchanged too. Their real rhythm/speaker constructors remain unavailable until those actual systems exist; no empty converter is published. ME voice requests and controller-speaker playback use the existing explicitly disabled object-output policy. Controller speaker is not playable, so original SoundUtil follows its ordinary system-SE fallback. Four AudSystem member entry points retain explicit missing-owner errors, matching AudWrap::getSystem's existing failure before any current caller reaches them.

The new JAudioPlaybackService method loads its already-supported actual retail archive without opening an audio device or loading any wave archives. RuntimeContext's ONLY audio-owned hunk changes the existing make_disabled_object_audio_service call to pass `_j_audio_playback.get()`. Other RuntimeContext dirty hunks belong to earlier DrawSync integration and must not be attributed to this checkpoint.

## Validation

- All 11 production/source import translation units pass isolated LLVM 23 native syntax. Receipt: `syntax-first.json`.
- New original-owner fixture syntax passes: `name-owner-test-syntax.json`.
- Standalone ASan+UBSan oracle compiles the actual original converter, original SDK table and original JGadget hash alongside the changed bounded Aurora adapter. It verifies **3,484 original-category sound lookups** against independently read big-endian retail BSTN records; zero wave/device calls. `name-oracle-run.json` records executable SHA and output.
- Three internal `SE_DUMMY_SGxx` identities have no original named-call category prefix and are deliberately not invoked through the original API. An initial overly broad oracle exposed the original negative category index when those invalid callers were tested; production Game semantics were not changed.
- Original lookup starts in the category encoded by a name and searches later entries. It is not equivalent to the existing archive reader's global first-match lookup. The retail archive includes aliases such as `SE_SM_SIGNBOARD_HEY` physically in SV: original SM-category lookup returns anonymous. The final oracle preserves this behavior.
- `smg-pc-original-jai-sound-ownership-tests --names-only` now exercises real retail IDs, original named-SE dispatch and speaker fallback, nested singleton restoration, malformed-resource rollback, three heap reclamations, and no device/wave access. Root rebuilt the current shared target and ran this real-disc mode successfully; see `root-names-build.json` and `root-names-run.json`. This is component ownership evidence from the working integration, not original GameSystem startup or the demo sequence.

## Build integration and ownership

Root was asked to add SDK source entries `../JSystem/JAudio2/JAUSoundTable.cpp` and `../JSystem/JGadget/hashcode.cpp` to Game/xmake. Three Game converter translation units use the existing glob. The exact imported-file manifest is `original-imports.json`; those imports are byte-identical to existing decomp source/header files, so no new reference recovery was needed. `AudSingletonHolder::exchange` is a TARGET_PC owner-publication/retirement API only; original constructors and lookup methods stay unchanged.

`source-manifest.json` freezes all owned source paths (RuntimeContext is shared and its whole-file hash also includes root's prior DrawSync work). No GPU cohort source or shared build configuration was edited by this agent. This is named-lookup/disabled-output closure, not audio playback completion or gameplay proof.
