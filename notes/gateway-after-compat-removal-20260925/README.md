# Gateway after compat and scene removal

Both `src/compat/` and `src/scene/` remain deleted. Baseline `a71fc2ea7` now has a fresh controller-only route through all three rabbit catches and Rosalina spawning: 18000 completed frames, exit 0, 327.151 seconds, no remaining game process. The three post-catch conversations completed at frames 3540, 4730 and 6630; Rosalina became alive/unhidden at 7100. See `route-evidence.md`, `route-evidence.json` and `route.json` for exact observations.

The frame16000 screenshot shows Mario and a Luma on the tower stairs, with part of Rosalina above the parapet. A straight controller approach stops roughly 1130 units from her against the tower geometry. This is not evidence of broken collision, successful stair ascent, an unobstructed Rosalina, or retail visual parity. Grand Star completion remains unverified.

## Donor behavior restored

- `XanimePlayer` samples its saved pre-advance frame during the original calculation phase and restores the original initial interpolation countdown. Existing fixture expectations were adjusted to that donor contract; they were not run.
- `DemoExecutor` invalidates clipping for actors participating in a demo, matching its existing end-of-demo revalidation.
- `TalkBalloon` uses the original vertical reference vector for its beak angle.
- `RunawayTico` initializes its fallback color from the object argument; the old path could use an uninitialized value when demo cast registration failed.
- `RosettaDemoHeavensDoor` includes the original first spin-demo part in its sound predicate. Audio output remains disabled.

These are small restorations from the existing decomp donor, not new decompilation or Gateway exceptions. Native lifetime, archive and architecture adaptations remain intact. The audits document the exact source differences and their limits.

## Bounded validation

The full route ran the baseline executable before these edits. Afterward, one incremental application build passed in 4.567 seconds, followed by a fresh 120-frame Gateway opening/shutdown smoke: exit 0 in 3.031 seconds with no remaining process. No fixture suite was built or run. `validation.json` keeps the two binaries and evidence scopes separate.

The route operator uses normal button/stick input; no actor, switch, nerve, position or game-memory writes. The post-catch approach uses the copied `follow_actor.py` targeting the actual run's Rosetta ID1060 for frames13000–17700. The saved input file is the final neutral command, not a standalone route replay; accepted revisions are in `route.log`, and the operator scripts/logs record the controller choices.

## Evidence retention and next work

Exact milestone trace snapshots and controller logs are archived alongside the screenshot. The full112MB actor trace and its22MB gzip remain local; their hashes are in `artifact-manifest.json`. Restore the committed image with `gzip -dk route-frame16000.png.gz` to inspect it. Fresh save directories and process/index snapshots are local only.

The bounded post-Rosalina source audit found the original spin tutorial, crystal cage, launch driver and path implementations present. The next gameplay gate is ordinary stair ascent and Rosalina's spin tutorial, which requires fresh A triggers; the actual X binding supplies the controller swing gesture. No production movement or camera override was justified by these audits.
