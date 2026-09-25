# Audio linker closure under disabled output policy

Enabled the complete existing donor `AudAudience`, `AudEffector`, `AudMicWrap`, `AudRemixMgr`, and `AudRemixSequencer` units. No fake implementations were introduced for their reported methods. The corresponding source exclusions are removed in Game xmake.

Enabled actual `AudSystem.cpp` with a narrow native policy branch. The three listener methods (`setMicMtx`, `getMicPos`, `setFarCamera`) retain their exact donor bodies. The four existing unavailable-owner exceptions (`getChordInfo`, `setSeVolumeSet`, `recoverSeVolumeSet`, `registerLimitedSound`) moved from AudioFacadeCompat onto AudSystem. The complete original DSP constructor, factory and control implementation remains unchanged in the non-PC branch. The native process still does not construct or publish an AudSystem. This bounds the requested linker closure without claiming functioning DSP output.

`AudSystemWrapper::isOutputDisabled` reads the actual process's existing wrapper/backend and returns true only for its explicit disabled backend. It owns no substitute publication/state. Native guards use that policy at output-only owners: AudCameraWatcher movement/atmosphere, AudEffectDirector effect parameters, SoundEmitterCube/Sphere movement, AudMicWrap setters, and SoundUtil atmosphere/remix submissions. Original non-PC callbacks remain intact. AudMicWrap microphone position queries stay strict; they do not fabricate a camera/listener position. Remix note-count metadata stays strict because it affects actual gameplay and cannot be replaced with a fake count.

Inactive-audio-sources.json now lists 32 of the initial 38 imported units, with the six activations explained in the round README. Existing audio compatibility providers and unavailable full DSP construction remain explicitly incomplete.

Snapshots and the isolated audio-only.patch preserve preexisting root/agent edits, including the seven preceding FileSelector/Mii source activations. No tests, builds, new fixtures or Git state mutations were performed. Whitespace checks passed. Root owns the integrated app build and short runtime smoke.
