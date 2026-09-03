# Gateway compatibility expansion

Goal: execute original Game code with minimal host-specific changes, working
toward Gateway Galaxy's bunny chase through Rosalina appearing. Compatibility
fixes belong in Aurora or the host services wherever possible. Newly recovered
game functions must be implemented at the repository root and mirrored into
the port, following `AGENT_DECOMP_GUIDE.md`.

## Starting state

- Parent branch `pcp-aurora`, HEAD `d8a459932`.
- Aurora is checked out at `pc-port/aurora`, pinned at `87849d17`.
- Existing macOS package/build, launcher, MSL functional adapter, and generic
  animation-publication work is uncommitted. Preserve and review it before
  checkpointing. Initial tracked patches are saved under `.git/` locally.
- The supplied Korean RVZ stays local and must never be included in commits.
- Earlier notes establish a bounded native Mario walk scene; they do not
  establish completion of the bunny chase. Refresh runtime evidence here.

## Work log

- Read repository compatibility and decompilation guidance. Parallel audits
  cover the actual Gateway route, Aurora gaps, and the pending macOS checkpoint.
- Published the macOS baseline as parent `3f169e732`, pinning Aurora `9c9b1a4`
  (branch `codex/macos-compat`). Both pushes completed successfully. Included
  package recipes, launcher, LLVM portability, and animation publication.
- The ordinary showcase keeps story progress 5. The `gateway-spin` route is a
  separate progress-10 development checkpoint with substitute demo casts and
  does not prove the chase. Do not expand that mechanism into production.
- The factory currently activates `DemoRabbit`, but not `RunawayRabbitCollect`,
  `RunawayRabbit`, `RunawayTico`, or `Rosetta`. General subsystem work is needed
  before those real actor constructors can be enabled.
- Source-backed scheduling work separates movement suspension from animation
  and drawing, and exposes the original category movement request APIs.
- Math work replaces host approximations with recovered vector/quaternion
  algorithms and their general JGeometry prerequisites.
- Recovered collector bodies were lost during the upstream merge; restore from
  `96e5ef0` and reconcile only upstream field names, with compile evidence.

## Next system boundaries

- Programmable demos require real movement masks, request arbitration,
  cinema-frame state, and player control ownership. Merely replacing the
  `DemoUtilCompat` exceptions with a DemoSheet start is not sufficient.
- Joint controllers require an actual J3D joint callback pipeline, including
  parent-before-child and after-child phases and the published animation
  matrices. `NPCActorRuntimeCompat` currently rejects this surface.
- Star-pointer targets need the complete pointer/matrix position binding
  contract from root `StarPointerTarget::calcPosition`, including rotated
  offsets and live joint matrices. The current actor-position-only service is
  insufficient for unmodified NPC initialization. Avoid an actor-specific
  `initStarPointerTargetAtJoint` substitute.
- The production player still lacks the original sensor initialization and
  full state closure. The collector's catch sensor must reach real player
  sensors through ordinary overlap dispatch; scripted catch counters would
  not establish gameplay.

## Validation and limitations

- Current native showcase build: PASS.
- Real-disc Gateway smoke: PASS, 28 rendered frames, animated Mario/GPU packets,
  gravity acceleration, and authored planet KCL contact.
- Real-disc Mario stand/walk/release/recreation: PASS, 325.685 units travelled,
  `Wait -> Run -> Wait`, 14,521 KCL triangles.
- Generic animation publication: PASS, 6/6, including derived `calcAnim` and
  retained joint matrices.
- Captured and visually inspected `baseline-gateway.png` at frame 35. The
  supported view renders Mario and the planet; it is not a bunny-route proof.
- Further current-run system validation pending below. No complete chase or
  Rosalina appearance is claimed.

## System checkpoint verification

- Scene movement category test: PASS 6/6.
- Quaternion/vector rotation and interpolation test: PASS.
- Real-disc paused model: PASS, 12 Tico 3D packets and four 2D packets, actual
  GPU draw, retained Body joint refresh, frozen BCK 4 and one-tick resume to 5.
- DemoSceneRuntime synthetic graph/lifecycle tests: PASS 19/19; its optional
  real-disc definition fixture was skipped by that test.
- Gravity math foundation: PASS.
- NPCActor: PASS 6/6.
- Current real-disc Mario stand/walk/release/recreation: PASS at 325.685 units.
- Existing Game and Player source mirror tests: PASS.

Two stale tests initially failed. The gravity fixture compared table-quantized
cosine with ideal cosine; it now uses the independently known retail table
cell. The demo fixture omitted child-holder traversal identity introduced by
the earlier placement-order work; it now declares that ownership. No runtime
behavior was altered to satisfy either stale expectation.

Root actor restoration was published as `bac110f54`. The runnable actor
factory has not been broadened by this checkpoint.
