# Original JAudio sound and stream owner publication — 2026-09-10

Published the interrupted September 7 draft after verifying all 56 previous destination hashes against the worktree. The complete original JAISound, JAISoundHandle, sound child, starter, audience, and JAIStream/JAIStreamMgr objects now replace the old Aurora token-handle header. The single Game header change restores all six original AudSingleBgm inline methods in AudBgm.hpp; it is byte-identical to decomp/include/Game/AudioLib/AudBgm.hpp.

The ten existing reference implementation bodies are copied to compat/jaudio. Seven are byte-identical; JAISound changes only the host-pointer diagnostic, and JAIStream/JAIStreamMgr preserve ARAM addresses with uintptr_t. Native SDK headers adapt pointer width and PPC bitfield/endian storage. reference-proof.json verifies the current reference source hashes still match the earlier Wii compilation/objdiff evidence; those reference builds were not rerun today.

JaiStreamPlayback owns an actual JAIStreamMgr, its original three-stream pool, child pool, ARAM allocations, and the real original preparation/calc/mix/free lifecycle. The platform backend decodes real STRM resources and connects JASAramStream message queues/callbacks to Aurora PCM. Effects use a clearly native NativePcmSound subclass of complete JAISound; asSe/asSeq/asStream remain null. No JAISe, sequencer, or AudSystem object is fabricated. Full AudSystem and JAS sequence playback remain separate unfinished work.

The migration preserves the single reciprocal original sound handle. AudSingleBgm adopts that handle rather than copying backend tokens. Follow-up review fixed repeated adoption stopping its own sound, full facade synchronization releasing live handles, and reset_scene closing an already-open audio frame. Tests now use actual asynchronous preparation and frame-driven faders: LOCK_READY has no playing PCM voice, isPrepared includes PLAYING, and explicit pause freezes the original fader.

## Verification

- 22 direct translation units compiled against the published src/ and aurora/ include paths, without draft overlays. compile.json retains exact commands. September 7 Xmake package hashes had expired; the probe resolves the current installed package paths.
- Aurora controls test compiles, links, and runs successfully from the actual published audio.cpp. It checks independent channel volume/pan, pitch, paused source position, rejected-update isolation, preserved pending release, and retired-token detection. pcm-result.json and pcm-run.log record the fresh evidence.
- Aurora commit fecf30485a505f18b51e2ec10f1596eb8aa001ae was authored/committed as codex, pushed, and verified against origin/codex/macos-compat.
- Fresh standalone original JAISound/NativePcmSound compilation, linking, and execution all passed. The test takes the actual native-owner assertions from OriginalJaiSoundOwnershipTests.cpp; numeric PPC flags, same-ID handle replacement, destruction-detach, pause-frozen fade, and actual PCM completion passed. sound-core-result.json records this independent run.
- Fresh official xmake build -v smg-pc-original-jai-sound-ownership-tests and execution both passed (xmake-result.json and xmake-run.log). The real SMR.szs/STRM fixture was explicitly selected through SMGPC_RETAIL_FILES_ROOT; all four retail/owner pass lines ran with no skipped fixture. This rebuilt the debug Game/render/common and current SDK declaration closure before linking. Initial attempts failed because the build directory was absent, then because the concurrently imported Game pause children lacked their17 SDK declarations; both conditions were resolved before this successful run.
- The broader JAudioPlaybackTests.cpp and exception test were adapted and compile; their full runtime result is not implied by the focused ownership test.
- These audio checks do not establish playable Mario jumping, the full game camera, or Gateway bunny chase completion.

The root source manifest excludes other agents' scene/graphics work, user-staged deletions, and the untracked demo packager. Aurora's graphics checkpoint precedes this audio checkpoint and is preserved.
