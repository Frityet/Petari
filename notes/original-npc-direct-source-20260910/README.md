# Direct original NPCActor source — 2026-09-10

The user explicitly requested removal of `NPCActorSource.inl`, permitting necessary direct edits to `NPCActor.cpp` instead of preprocessor substitution. This supersedes the older blanket restriction for this narrow compiler fix; it does not authorize rewriting NPC behavior.

## Change

- Compile `src/Game/NPC/NPCActor.cpp` directly in the normal Game archive. The only source difference from the reference is qualifying `&NPCActor::calcJointScale`, required by standard C++.
- Delete `NPCActorCompat.cpp` and `NPCActorSource.inl`, including all token replacement and private-member access tricks. All prior extra declarations are already present in current utility headers.
- Read the actual original `mModelManager` and `mSpine` fields directly. They are already public and backed by original owners; their wrapper substitutions no longer served a purpose.
- Enable the complete original `JointController.cpp` against the current actual J3D model, joint and matrix buffer. Remove the unconditional joint callback/factory rejection and obsolete model-presence helper from `NPCActorRuntimeCompat.cpp`.
- Reconcile the complete earlier joint-controller recovery into `decomp/` before publishing. The current reference lacked five explicit methods and the emitted model accessor, and retained an obsolete partial info structure. Fresh Wii compilation/objdiff confirms all ten functions, 684 text bytes and 16 data bytes at 100%; see `joint-reference/`.

No NPC action, reaction, item, talk or demo substitute is added. The Gateway sequence remains incomplete. The existing packaged movement demo is unchanged by this source cleanup.

## Verification

- Direct source before: one compiler error, the unqualified member-function address (`direct-source-before.*`).
- Direct source after: syntax passes without wrapper definitions (`direct-source-after.*`).
- Initial native build compiled both original TUs, then exposed a stale NPC fixture reference to the removed `actor_base_matrix` registry (`build-npc.log`). The fixture now uses actual Tico resources and the original animated-model getter, with virtual-call observations measured around the specific operation. Player-dependent float tests now use the already initialized actual Mario in the PlayerUtil fixture. No removed registry was restored.
- `smg-pc-game`, NPC tests, joint-controller tests, original traversal tests, actual-Mario tests and showcase all build and link. The game archive contains exactly one `NPCActor.cpp.o`, exactly one `JointController.cpp.o` and no old NPC wrapper object (`archive-source-selection.json`).
- `smg-pc-npc-actor-real-or-absent-tests`: **6/6** groups pass, including real Spine transitions and real Tico model/base-matrix ownership (`run-npc-final.log`).
- `smg-pc-original-joint-controller-tests`: **2/2** SDK groups and the real NPC resource group pass. All four pre/post acceptance combinations, null hooks, unknown phases, callback retirement, descendant/sibling propagation and two real Tico models sharing resource joints are covered. Two complete scene generations release their actors, registrations and heap (`run-joint.log`).
- `smg-pc-original-j3d-joint-traversal-tests`: **9/9** groups pass (`run-traversal.log`).
- `smg-pc-mario-gateway-walk-tests --player-util`: passes, including all relocated talk-height assertions and an additional actual-player-up change. Completes original player initialization, finite joints/draw buffers and scene teardown (`run-player.log`). This is the focused option, not a full default walk/chase test claim.
- Current showcase binary SHA256 `0e81c752a21e177e4002a5e055e44100fae33e68d955a88c6c294303c837a4c6`: **960 frames, 1280x720, 13/13 synthetic SDL input checks, 59.942 presented FPS, exit 0**. Walking in all four directions, key releases, finite camera/movement, and both jumps/landings pass. See `movement-summary.json`, `movement-validation.json`, and the replay under `../preview-fps-crash-20260910/original-npc-direct-source-final.*`.
- NPC header and both JointController files are byte-identical to decomp; NPC source has only the one standard C++ qualification (`source-closeness.json`).

The packaged movement app was not replaced; this is source/runtime validation of the cleanup. No full Gateway chase or Rosalina completion is claimed. Large evidence logs/reports are committed as `.gz` siblings with original hashes in `compressed-evidence.json`.

## Publication

Decomp `8f8633065b1b0c73ae5d49f70b9a72dea64ffcb4` was committed and pushed to `origin/pcp-decomp`; the remote SHA and codex author/committer were verified. It includes the original joint-controller implementation/header and two typed boss consumer reconciliations that preserve every code/data section exactly.

Root implementation/evidence checkpoint `f2ed2ebc8f074aa1083d28d29f7e942fe1c5de71` was pushed to `origin/pcp-aurora`, with an exact remote SHA verification. Both author and committer are `codex <codex@openai.com>`. Independent review found no actionable defects in the scoped production/test change. The four protected user paths retain their original status, including both staged deletions; the untracked walking-demo script retains SHA256 `60f326015cf765503100cd142f386bde95b24be72ccc6bf94c85b8879fcf1ced`. Aurora and decomp working trees are clean. The Gateway/Rosalina goal remains active and incomplete.

## Preserved user work

The already staged `DISCREPENCY_REPORT.md` and `MACOS.md` deletions, dirty `notes/original-sequence-galaxy-move-20260910/README.md`, and untracked `script/package_walking_demo.py` are outside this checkpoint. Commits will be scoped and authored/committed by `codex <codex@openai.com>`.
