# Remove audio compatibility and preview playback

All 24 remaining audio compatibility files are deleted. Only ActorRuntimeRegistry and JkrAllocationDomain (two files each) remain in src/compat; src/scene remains deleted.

Actual AudSystemWrapper, AudSceneMgr, AudSoundObjHolder and AudSoundNameConverter now own initialization, name decoding and sound-object bookkeeping. Game audio, BGM and animation owners compile their original algorithms with targeted disabled-output guards. The canonical JAU sound-object and BAS animation scheduler replace the handwritten compatibility implementations. SDK heaps, memory pools and stream entry points live at their JSystem owners; pointer-width corrections preserve native allocation links.

The synthetic runtime playback/event service and trace output are removed, as are orphaned scenario/particle owners and tests exclusively exercising deleted preview implementations. No native DSP, speaker, rhythm or voice owner is fabricated. Missing hardware operations decline starts or explicitly reject access at their actual owner.

Aurora exposes WPAD speaker availability and configured volume using the existing host-device policy and system configuration. No Game dependency was added to Aurora.

Validation is limited to an integrated application build and one bounded opening run; results will be recorded below. The wakeup-to-Rosalina demo and audio playback are not established by this batch.

The integrated build passed on the fourth compile/link attempt (final incremental build: 5.05 seconds). The initial file-flag conflict and newly exposed audio links were corrected. One fresh-save, neutral-input Metal Gateway run completed 120 frames, exited 0 in 3.132 seconds, and left no process. Binary SHA-256: 8deec99ea0987ed2be7af943f74a8f45f42d0292107f97793a76a4e329352ebf. No fixture suites ran.
