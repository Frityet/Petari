# Original JAudio sound output

Goal: restore original decompiled sound managers, sequence interpreter, instruments, channel allocation and envelopes. Host compatibility belongs at resource byte order, address width and DSP/device boundaries. Preserve the user-controlled live process.

Baseline: stream music works; JAISeMgr and JAISeqMgr return false on PC, JASTrack and JASSeqCtrl have only partial implementations, and no AudSystem is constructed.

## Integration checkpoint

Restored the donor AudSystem/AudWrap/AudSceneMgr and original rhythm and speaker owners, plus the original JAI SE/sequence managers, sequence interpreter, bank/wave parsers, envelope/channel implementation and resource loaders. Imported existing donor sources directly; no newly invented game sound recipes. Native fixes cover big-endian archive operands, pointer-sized resource relocation and message payloads, offsetof-based intrusive nodes, native PPC bitfields, and original DVD/ARAM transport.

Aurora now contains the Dolphin Zelda/JAudio DSP renderer adapted to a bounded memory interface, and an SDL signed-PCM DMA output. Original JAudio controls channels; the native renderer performs the DSP sample work. The initial scheduling implementation advances original 80-sample subframes from the game frame loop; independence from render cadence still needs validation/improvement.

The previous Terrace user session exited normally with status 0 after 35,910 frames; no live controls or process were interrupted. New tests use copies of saves and no input injection. Compilation checks of 70+ restored units pass; full executable link/runtime validation is still in progress.

## Verified checkpoint

The 600-frame real-disc Good Egg probe accepted SE_SY_BUTTON_CURSOR_ON, SE_SY_TALK_OK, and SE_SY_GALAXY_DECIDE_CANCEL through original JAI handles, produced 483,506 nonzero DSP samples before teardown, and exited with status 0. The separate stream mixer produced zero samples during this interval, so this measurement comes from the restored DSP path. It establishes real sequence/bank/channel/sample output to SDL, not a listening-quality judgment.

Two general boundary bugs surfaced and were fixed: signed 32-bit overflow in OSNanosecondsToTicks(6666667), and static JAS pools retaining pointers after their JKR arena retired. Timing operands now widen before arithmetic. Each pool registers an arena disposer that clears its slots before reclamation; the probe exercises retirement and reallocation in two arenas. PPC bus-routing and audience-bitfield ordering also have focused assertions.

Remaining immediate work: decouple DSP scheduling from rendering, verify the original intro reaches gameplay with music plus SFX, then publish the playable build.
