# Original JAudio sound output

Goal: restore original decompiled sound managers, sequence interpreter, instruments, channel allocation and envelopes. Host compatibility belongs at resource byte order, address width and DSP/device boundaries. Preserve the user-controlled live process.

Baseline: stream music works; JAISeMgr and JAISeqMgr return false on PC, JASTrack and JASSeqCtrl have only partial implementations, and no AudSystem is constructed.

## Integration checkpoint

Restored the donor AudSystem/AudWrap/AudSceneMgr and original rhythm and speaker owners, plus the original JAI SE/sequence managers, sequence interpreter, bank/wave parsers, envelope/channel implementation and resource loaders. Imported existing donor sources directly; no newly invented game sound recipes. Native fixes cover big-endian archive operands, pointer-sized resource relocation and message payloads, offsetof-based intrusive nodes, native PPC bitfields, and original DVD/ARAM transport.

Aurora contains the Dolphin Zelda/JAudio DSP renderer adapted to a bounded memory interface, and an SDL signed-PCM DMA output. Original JAudio controls channels; the native renderer performs the DSP sample work. The initial checkpoint advanced subframes from the game frame loop; the final checkpoint below replaces this with device-driven scheduling.

The previous Terrace user session exited normally with status 0 after 35,910 frames; no live controls or process were interrupted. New tests use copies of saves and no input injection. Compilation checks of 70+ restored units and the full executable pass.

## Verified checkpoint

The 600-frame real-disc Good Egg probe accepted SE_SY_BUTTON_CURSOR_ON, SE_SY_TALK_OK, and SE_SY_GALAXY_DECIDE_CANCEL through original JAI handles, produced 483,506 nonzero DSP samples before teardown, and exited with status 0. The separate stream mixer produced zero samples during this interval, so this measurement comes from the restored DSP path. It establishes real sequence/bank/channel/sample output to SDL, not a listening-quality judgment.

Two general boundary bugs surfaced and were fixed: signed 32-bit overflow in OSNanosecondsToTicks(6666667), and static JAS pools retaining pointers after their JKR arena retired. Timing operands now widen before arithmetic. Each pool registers an arena disposer that clears its slots before reclamation; the probe exercises retirement and reallocation in two arenas. PPC bus-routing and audience-bitfield ordering also have focused assertions.

## Original streaming and independent audio clock

Removed the native stream recipe/mixer branches from JAIStream and JAIStreamMgr. Both implementation files now match their donor sources except for pointer-width ARAM casts. The original JASAramStream prepares music through DVD tasks, loads ARAM blocks, owns stereo JAS channels, and handles pause, resume, fades and release. Its on-disc headers use explicit big-endian fields; the encoded samples remain unchanged. Music and SFX share the original DSP channel/mix path and one SDL output device.

A host worker schedules the original CPU/DSP subframe order from device queue consumption, preserving the three-DMA-buffer lead. It acquires the same guest execution lock as the game and DVD threads. Rendering waits no longer stall the audio clock. Original external mix callbacks (including the THP callback) run at DMA cadence. Shutdown stops sound owners, joins the DSP/DVD workers, and clears callbacks before audio heaps retire.

Restored the original melody/speaker name owners, speaker resource publication, and wrapper wave-load/reset methods. Resource conversion handles native pointer tables and big-endian PCM at the boundary. Keyboard/mouse has no Wii remote speaker; the WPAD API reports that capability accurately. No replacement speaker sounds are synthesized.

The ringing fix is documented in [ringing.md](ringing.md): JKRArchive buffer reads were passing Yaz0 sequence bytes to the original interpreter. Original archive-member decompression eliminates the sustained 2.79 kHz voice without filtering or suppressing a sound ID.

## Final validation

- `original-stream-gameplay.log`: real-disc Good Egg opening ran for 1,800 frames, reached original SceneAction at frame 562, and exited 0. Original stream channels carried PCM16 data across multiple DVD/ARAM blocks. The shared DSP submitted 1,097,600 stereo frames with 1,994,828 nonzero samples.
- `original-stream-pause.log`: 1,200-frame real-disc probe accepted three named UI sounds through original JAI handles, verified stereo streaming and multiple DVD blocks, paused/resumed the original transport, and verified audio advancing while the render caller waited 100 ms. It exited 0 with 1,359,176 nonzero DSP samples at teardown.
- `original-sound-ownership.log`: original stream pool allocation, all six child owners, prepare locks, cancellation before DVD preparation, and pool reuse passed across three cycles.
- `ring-archive-tests.log`: ten archive regression groups passed, including fresh Yaz0/Yay0 member reads, bounded partial reads, ID/path lookup and encoded/expanded lengths.
- `final-build.log`: main executable and focused audio tests built with LLVM 23 on macOS ARM64. Source diffs passed whitespace checks.

These checks establish execution, sample output and the measured ringing fix. They do not establish listening quality for every effect, every cutscene, or every audio device. Save-backed diagnostics copied the user's two-star save; no test injected live controls or altered their progress.
