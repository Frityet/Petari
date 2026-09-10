# Original galaxy transition request recovery

Recovered `GameSequenceProgress::requestGalaxyMove(const GalaxyMoveArgument&)` in the decompilation reference. The native port source has not been changed or integrated by this task.

The implementation follows `decomp/AGENT_DECOMP_GUIDE.md`, the surrounding `GameSequenceProgress.cpp` conventions, and the retail instructions at `notes/gateway-audit-20260907/restoration/retail/asm/Game/System/GameSequenceProgress.s:334–516`. It adds the existing `GalaxyStatusAccessor.hpp` include and fills the one missing method; it adds no compatibility substitutes or new headers.

## Evidence

- Fresh Wii compiler invocation exits 0: `wii-compile.json` and `wii-compile.log`.
- Fresh retail objdiff exits 0: **94.47305%**, retail 668 bytes, candidate 632 bytes. `retail-proof.json` records the exact commands, source/object hashes, and every remaining instruction difference; the full machine-readable output is `objdiff.json`.
- The nine deleted instructions are a second 24-byte argument copy in the retail retry/comet query. The remaining differences are stack-frame sizes and stack offsets caused by that omitted copy. The call targets, boolean logic, dispatch branches, and stores match after relocation/stack normalization. The direct field query retains the behavior without introducing an invented helper solely to force the extra copy.
- `git -C decomp diff --check` exits 0.
- `source-manifest.json` records the reference revision and exact changed source hash.

These are compilation, retail comparison, and source/assembly review results. The transition has not been linked into the native game or exercised at runtime here.

## Retail behavior retained

1. Update the visited-galaxy flag and store the scene-start game data. Move type 4 additionally processes the cleared stage result, updates Finding Luigi with the cleared stage/star, and counts down game-event values.
2. Update Finding Luigi using the original incoming argument. Copy that argument for the story sequence to modify, using reset-processing or the scenario-cancel flag as its boolean parameter.
3. Synchronize comet flags, update game data, and compute the minimum wait before the next stage from the modified argument.
4. Populate the real scene controller's next scene with `Game`, stage name, scenario number, selected scenario number, and start ID. Request the scene change, then reset per-transition game data.
5. Move type 2 starts foreground scenario selection and sets the star pointer base mode. Type 7 starts background scenario selection and selects the title pointer state. Type 5 shows the remaining-lives layout only when it exists and the destination is not a comet star, then follows the normal background-selection path. Types 0, 1, 3, 4, and 6 share that normal path; type 6 also synchronizes Luigi's remaining-lives supplier. Other move types skip these branches.
6. Set `_26`, clear it for move type 2 or `EpilogueDemoStage`, and enter the original galaxy-move nerve.

The scene request uses the argument after `StorySequenceExecutor::moveGalaxy` modifies it, including its move type. Preserving this ordering matters for story-driven destination changes.

## Publication scope

Only `decomp/src/Game/System/GameSequenceProgress.cpp` is changed. Root integration and the shared decompilation checkpoint belong to the parent task; no commit or push was made here. Compiled objects, the first-candidate scratch source, and the large objdiff output are supporting local artifacts, not required source additions.
