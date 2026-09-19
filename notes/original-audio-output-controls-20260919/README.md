# Actual audio output controls at the original utility boundary

After enabling original DemoGroup placement, the actual TicoGuideDemo progressed through its 930-frame wake-up, 75-frame Tico transformation, 75-frame rabbit appearance and 15-frame Mario animation. The first rabbit conversation paused the authored demo normally, then TalkBalloonEvent::exeOpen invoked MR::setSoundVolumeSetting(3,30). The old SoundUtil forwarded this output control to an absent AudSystem and threw. Exact stack and authored progression are in ../gateway-compat-20260919/opening-progression.log.

The native SoundUtil provider now lives in compat/OriginalSoundUtil.cpp. Its canonical Game/Util/SoundUtil.cpp remains unchanged and is excluded from native compilation. The complete original helper bodies are preserved except the seven functions for volume preset/recovery and trigger/level SE permission controls. These dispatch to the actual existing disabled-object-audio service when present. Without its owner the original AudSystem boundary still rejects unsupported access. No Talk, Demo, Player or other gameplay body changed.

The service retains the existing genuine AudSystemVolumeController and sixteen JAISoundParamsMove category records through JAudioCategoryVolumeOwnership. It preserves original nested preset push/recover behavior, fade steps and table values while creating no AudSystem, JAI voice or output device. The original wrapper movement advances that logical state; scene start resets volume and SE permission as the original AudSceneMgr operation requires. A standalone RuntimeContext owner delegates to its existing concrete PCM backend rather than creating competing output parameters.

The volume ownership registry is shared across the process under the existing guest CPU gate. Original audio initialization happens on an OS worker and scene/dialogue control on the main guest; thread-local ownership lookup incorrectly hid an otherwise live original controller. Its lifetime still ends with the real audio owner.

source-equivalence.json records the provider comparison: only the seven intended function bodies differ from the canonical native Game utility. Tests extend OriginalJaiSoundOwnershipTests --scene-only with real table-derived gain/fade expectations, nested volume recovery, independent SE permissions and scene reset. --backend-only constructs on the actual original OS worker and verifies main-guest volume mutation, wrapper-driven fading, recovery and three heap retirements. Parent coordinates native builds and execution. Limited-sound, chord and other unsupported AudSystem access remain explicit boundaries.

For a concrete PCM backend, resetting scene output controls resets its existing category parameters and permission flags only; it does not destroy playing voices. The original AudSystem::initSceneVolume adjusts controls without a scene-wide sound stop. JAudioPlaybackService exposes this narrow reset separately from its existing full scene teardown.

## Verification

The native production build passed. The resulting static archive contains
`OriginalSoundUtil.cpp.o` and no stale `SoundUtil.cpp.o`. The original source
copy therefore remains reference-only for this provider.

- `scene-test.log`: the scene-only owner test passes, including independent
  SE permission flags, original preset interpolation, nested recovery and scene
  reset.
- `backend-test.log`: the backend-only test passes against the retained retail
  audio fixture. This constructs the owner on the actual original OS worker and
  then mutates, advances and recovers its volume on the main guest, across three
  owner/heap lifetimes.
- `volume-test.log`: the existing category-volume regression passes for all
  eight presets, sixteen categories, fade steps, nested recovery, level timeout,
  owner rejection and independent PCM bus gain during a stop fade.
- `../gateway-compat-20260919/dialogue-controls.json` and `.log`: the ordinary
  original-process HeavensDoorGalaxy scenario 1 run completed all 1800 frames
  with exit 0 in 112.52 seconds. Binary SHA256:
  `532d5a8cf3502233bceab32f8354d95cb8acbcd79c26dad830828757e4b2764e`.
  This is a bounded no-input run past the previous first-dialogue exception;
  it does not establish dialogue confirmation, the rabbit chase or Rosalina.

A subsequent code review found that the new narrow PCM control reset also
needs to refresh already-playing voice bus gains immediately, as the existing
preset and recovery operations do. `reset_output_controls` now applies the
reset category gains directly. The existing retail playback test adds a check
that a real playing wind voice retains the same sound object and backend token,
remains active, restores its default category gain immediately and restores both
permission flags. Rebuild/retest of that final correction is pending; the run
above predates it.

The threading audit confirms that `run_managed_thread` acquires Aurora's
`sCpuGate` before the audio initialization callback and that original main-guest
frame execution uses the same gate. `GameSystem::updateSceneController` reaches
`GameSystemObjHolder::updateAudioSystem`, which invokes the original wrapper's
movement once per frame. No audio mixer callback traverses the category-owner
registry. This change supports serialized original guest ownership; it does not
claim a new freely concurrent native audio-owner API.

Final verification after the immediate PCM gain correction: the eighth native build passed; the complete OriginalJaiSoundOwnershipTests passed using the existing retail fixture, including the playing-voice identity/token/active-state and immediate default-gain assertion. SoundPermissionTests passed absent-owner rejection, trigger/level independence, original exempt categories, and scene reset. Logs: ownership-final-build.log, ownership-full-test.log, permission-build.log, permission-test.log. The live bundled app displayed the first authored rabbit message; direct Return presses did not yet establish conversation advancement (native window sampling is under separate investigation).
