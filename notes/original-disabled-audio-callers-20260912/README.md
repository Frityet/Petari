# Disabled Aud output at original scene/menu/error boundaries

The original startup link exposed five members of the disabled output graph:
`AudSceneMgr::startScene` and `AudSystem::enterPauseMenu`, `exitPauseMenu`,
`doDvdErrorProcess`, and `exitDvdErrorProcess`.

Their complete reference bodies are already present in
`decomp/src/Game/AudioLib/AudSceneMgr.cpp:364` and
`decomp/src/Game/AudioLib/AudSystem.cpp:539-557,614-626`. The scene method resets
audio flags/volume/effect parameters and reconnects the controller speaker.
Pause methods adjust audio state, faders, volume sets and voice pause state.
Disc-error methods similarly mute/stop/pause audio and later resume it. These
methods do not control scene, menu or error-window progression outside the Aud
output graph.

Four native Game callers now use an explicit `TARGET_PC` compile-time guard
around these five original calls. They all consult the existing
`aurora::audio::DisabledObjectAudio::enabled()` policy, already asserted false
by the actual `DisabledAudioBackend`. No new policy, readiness flag, fake
AudSystem/AudSceneMgr, member implementation or null-this call is introduced.
Changing that policy in the future restores the original calls and requires
their genuine output owner implementation to link.

The original non-PC branches are preserved verbatim. All scene progression,
pause-menu state changes, rumble, error-window state and ordinary named sound
requests outside these five calls remain unchanged. The existing independent
native PCM/BGM implementation is a separate audio boundary; these guards do
not claim to implement original Aud pause/volume/effect semantics for it.
Audio output remains outside the user's demo milestone.

## Validation and publication

- `native-compile.json` records isolated successful compilation of all four
  actual Game translation units and their undefined-symbol lists. None retains
  a reference to the five disabled members; no stub definitions were added.
  Existing header/attribute warnings remain recorded in the receipt.
- `reference-branch-proof.json` verifies that selecting the original branch
  reproduces each complete pre-edit native source byte-for-byte.
- `before/` and `owned-hunks.patch` preserve exactly these narrow edits for the
  parent to publish without adopting unrelated ongoing work.
- `source-manifest.json` freezes the four owned files. No audio name/backend
  fixture source, decomp file, shared build list or display source was changed.

The parent owns the next integrated startup link/run. These checks establish
the intended symbol closure, not successful full startup or Gateway gameplay.
