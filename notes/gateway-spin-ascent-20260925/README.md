# Gateway tower ascent, spin tutorial and launch-star reveal

The fresh-save route on baseline 412329362 completed all three rabbit catches, climbed the original tower stairs, showed an unobstructed Rosalina in her original spin-giving cutscene, returned control to Mario, broke the crystals with ordinary X input, and revealed the first launch star. The game completed 36000 frames and shut down with exit 0 in 630.148 seconds. The launch ride and Grand Star are not yet verified.

The route is input-only: read-only actor traces drive ordinary controller buttons/stick, and the normal X key binding supplies the shake gesture. No actor transforms, nerve states, story flags or switches were forced. See `route-evidence.md`, `ui-inputs.md`, the recorded controller commands, and the screenshot artifacts. The authored collision geometry supplied the stair waypoints; this navigation stays in notes and is not production game behavior.

## Original behavior restored in this checkpoint

- EarthenPipe recognizes its WaitToShowUp state and uses the original non-swimming, horizontal-distance entry guard.
- NPCActor converts turn degrees to radians with the original factor rather than the old ten-times value.
- PowerStar advances waiting rotation, writes its appearance interpolation to the actual position, and restores the original Koopa demo-model appear/kill order.
- StorySequenceExecutor scans each command until its terminator; hasNextDemo skips the current command.
- MarioActor restores the original nearest-rush threshold and selects the last occupied held-item slot when throwing.
- RushEndInfo restores the donor damage-type field using Aurora's existing full-word PPC bitfield declaration. PlayerUtil assigns the original six damage modes instead of clearing every mode to zero.

These are existing donor implementations restored on their actual owners. Native archive/lifetime changes and required architecture adaptations remain. No new decompilation, compatibility directory, level-specific branch or completion bypass was added; src/compat and src/scene remain absent.

## Validation boundary

The long route used the preceding signed application bundle, SHA-256 037dbc1201722dccc734c34e3e39eb1ff750fedde5f1599e0ea54be9854687b8. Its UUID matched the preceding standalone executable; signing explains the different file hash. The source changes were built only after that process was confirmed terminal.

One incremental application build passed in 6.318 seconds. The resulting standalone executable then passed a fresh 120-frame Gateway opening/shutdown check with exit 0 and no remaining process. `validation.json` records the exact two binaries, commands, timings and exit evidence. No fixture suite was built or run. Audio remains disabled and visual parity is not claimed.

The full actor trace and large frame 15000 readback remain local. Compact exact milestone snapshots, compressed controller logs, small native UI screenshots, audit notes, and process records are committed. UI JPEGs retain their original bytes and correct file extensions.

Next: ride the revealed launch star and exercise the original chip/key/cage/pipe/flip-panel chain through Grand Star pickup and the stage handoff. The later-owner audits identify restored donor differences but do not substitute for actual gameplay evidence.
