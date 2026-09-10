# Upstream integration and original-source cleanup — 2026-09-10

## Scope

Integrate SMGCommunity/Petari `16906807c69697fdea5d7efbb4642313c1b14204` (104 incoming commits since `151d5a53a`) while preserving verified local recoveries and the working movement demo. The reference is merged normally in `decomp/`; native sources are mapped individually into the flattened PC tree. Upstream Wii build/config files stay in the reference. Both repositories retain a `codex/pre-cleanup-upstream-20260910` safety branch.

User-staged deletion of DISCREPENCY_REPORT.md and MACOS.md, untracked script/package_walking_demo.py, and the unrelated existing sequence-publication note are excluded from this checkpoint.

## Concrete cleanup

- Remove fifteen duplicate MarioAccess/MarioActorGravity definitions from compatibility files. The original Game/Player translation units are already compiled and now supply these symbols alone.
- Restore the complete merged original AreaObjUtil source and header. Delete OriginalAreaMovement.cpp and CameraRepulsiveAreaUtilCompat.cpp: their three functions now come from their actual Game source file. Eight incoming original area helpers are included.
- Restore the original NameObjArchiveListCollector implementation and its getArchiveNum API; update native factory and test callers. This replaces a custom copy/null-handling implementation with the original MR::copyString behavior.
- Adopt original group/member and area-form names, original MarioParts source, and applicable source changes. NameObjGroup keeps upstream private members; one narrow friend permits the existing native lifetime registry to remove retired borrowed members. No old getter/field aliases are retained.
- Keep each previously verified Mario recovery where incoming partial source would regress behavior. Adopt the retail-confirmed physical-vector writeback gate. Correct upstream SwitchWatcher movement to the actual populated range captured before callbacks, validated at 100% Wii match.
- Recover LightAreaHolder::sort in the reference at 96.375% Wii match; it now exactly matches the existing native source, eliminating an unexplained port-only implementation.
- Remove thirteen redundant dereferences of an already-returned vector reference in decomp MarioCollision, then mirror it. The four affected Wii scores improve; the entire recovered checkGround function remains unchanged.

`native-dispositions.json` records individual native path choices. `decomp-merge-review.md` records reference conflicts, retained behavior and original compiler evidence. `original-provider-proof.json` verifies all eighteen migrated accessor/area symbols have exactly one strong provider in the actual Game archive, from their original translation units.

## Remaining replacements identified by review

1. PlayerUtil wrappers still maintain host copies for control, placement, grounded state and gravity despite the available actual MarioAccess owner. Move these as a coherent original-owner group and validate internal Mario/camera/sensor positions and reset timing.
2. CameraUtil projection helpers still calculate from a cached pose. Restore the original actual CameraContext projection/unprojection cohort, including projected depth, shake offsets and original screen dimensions. The live camera controller itself already uses original Game code.
3. Original WPad/GamePadUtil adoption needs complete Nunchuk KPAD publication, correct WPADProbe error-code semantics, original record updates and the missing retail WPadStick X/Y stores. Preserve working input until this generalized SDK/owner closure is tested.
4. Sky still has explicit unavailable SpaceInner/MirrorReflectionModel boundaries, and LightDirector retains native ownership/resource code. These remain source-restoration candidates. Do not replace missing children with game-specific substitutes.
5. Incoming implicit float-vector pointer aliases conflict with established native Vec pointer conversion at the preexisting Mario::decideInertia comparison. Retain the native conversion surface for now; verify the original comparison separately before altering movement semantics.

Detailed evidence and prerequisites: `compat-audit.md`, `input-audit.md`, `mario-overlap-audit.md`, and the independent mapped-source review notes.

## Validation

The updated native showcase builds with Homebrew LLVM 23 in optimized debug mode. Its exact binary SHA256 is `4ab87cc3db710b2c83d13a7b753c5f2df8e1105bd44c03f388b70f8c69ba12d1`.

The real-disc replay at the demo's 1280x720 size completed 960 ticks, all 13 checks passed, and the process exited normally: idle displacement zero, all four movement keys and releases accepted, two jumps landed, and all movement/camera poses remained finite. Presentation averaged 59.918 FPS. Input was synthetic SDL events through the regular input path, not direct Mario state changes.

276 impacted original Game translation units compile with the Wii compiler, plus the separately recovered LightAreaHolder. This is original-object evidence, not a full original executable link. All eighteen migrated accessor/area functions have a single strong original provider in the built native archive.

All 11 focused test suites build and pass. Focused fixture results are recorded in `validation-summary.json`. Older fixtures were updated to create actual scene/executor/catalog/camera/model owners and to obey GPU retirement. Source comparisons remain exact except the specifically checked two pure-virtual base declarations required by the native AreaForm header. No production missing-owner or null guard was weakened to make the tests pass.

The full Gateway bunny chase and Rosalina appearance remain incomplete.

## Publication procedure

Reference merge `ef44fad5dbc7def9d25cf149b823e1cf512bd3e2` is committed and published on `pcp-decomp`, authored and committed by codex; the remote branch SHA was verified. The root merge records the original native HEAD and exact SMGCommunity HEAD as parents with the explicitly mapped native tree. A separate Git index is used for this commit so the user's staged documentation deletions remain staged and excluded.
