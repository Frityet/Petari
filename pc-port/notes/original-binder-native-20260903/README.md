# Original Binder and movement math on PC

Started at root 2e6a86270 and Aurora deab54e on macOS arm64. Root source
recovery is committed and pushed as 7b0874008. This checkpoint removes the
native response solver and runs the complete recovered original Binder.

## Original Game source and native ownership

Game/LiveActor/Binder.cpp and Game/Map/CollisionCategorizedKeeper.cpp are
byte-identical to root. Three collision headers and MarioHang.hpp are also
identical. Binder.hpp differs only by a TARGET_PC destructor declaration;
compat/BinderCompat.cpp releases its original owned mPlane array at native
scene teardown. The former BinderParent layout, custom constructor, response,
classification, getters, sorting, and setExCollisionParts implementation are
removed. The root/PC MarioCollision member spelling is corrected to mPlane,
matching the actual Binder declaration. See source-parity.json.

StageCollisionService::move_sphere now constructs an actual Binder for its
geometry probes and scopes the live collision provider. It preserves and
restores the previous provider even if a filter throws. It uses no fabricated
Binder state or alternative response routine. The probe's downward gravity
only classifies contacts; probe callers consume the complete plane array.
Game actors use their real persistent Binders and gravity fields. The service
retains prism decoding/query responsibilities, not movement resolution.

All active-set and component-extrema duplicate solver code, sweep/retry code,
outer query margin and shell epsilon were deleted from the native service.
The original Binder owns stepping, input projection, component extrema,
classification, contact storage, and retries. Retail queries mRadius unchanged
and adds 1.2 to penetration only after an actual hit. Its one-shot _1EC._5
flag suppresses the margin and retry for that call. Its raw matrix columns
remain unnormalized when calculating a local offset. No stage-specific plane,
normal, actor-name, seating force, camera recipe, or special contact rule was
added. The old traced Gateway minimum-norm fixture is removed.

CollisionZone add/remove/bounds helpers are real source. The actual
CollisionParts::calcForceMovePower body and CollisionDirector getter are
provided without manufacturing owners. Native Triangle movement queries
recognize real parts and use their original matrix-delta calculation; static
registered KCL has no host motion. A full placed CollisionDirector and moving
CollisionParts lifecycle is still not active in the PC scene.

StageCollisionContact::moving_reaction represents collision-part motion
projected onto the face normal. Static registrations supply zero. Previously
reaction_normal duplicated the face normal and GameMapCollisionCompat copied
it incorrectly to HitInfo::_7C. Native Binder had left that field zero.

## Shared math and player lifecycle

MR::normalize vector overloads, isNearZero(vector), vecKillElement, and the
three vector polygon classification predicates use original source bodies.
vecKillElement projects with the caller's raw direction, without normalizing
it. Its existing paired-single helper has a root-first portable architecture
branch with explicit rounded Y/Z products, fused X accumulation/rejection,
and loads before writes for aliases. Volatile fused-result temporaries keep
ARM64's negated fused instruction from changing signed-zero results when the
console performs rounded fusion followed by negation. Native compile flags
preserve explicitly separated operations.

verify-math.py compiles the real GameMathCompat provider and compares 15,009
calls / 60,036 output values against decoded retail instructions at 0x803E7500.
All output bits agree for 5,003 finite moderate-range pairs in separate,
source-alias and direction-alias modes, including signed zeros and nonunit
directions. The harness links only the reachable actual helper. NaN payload/
sign, subnormal flushing and FPSCR are not asserted; see math-evidence.json.

Original MarioState lifecycle and base virtual bodies now supply real status
stack transitions, with no placeholder Wall/Hang/Swim/Wait instances. Original
compiler proofs and live-Mario callback tests are in the sibling
original-mario-state-lifecycle-20260903 directory. Its two exact-unit camera
fixture expectations were corrected to the retail SDK normalization result
0x3F7FFFFF, independently derived from the original estimate/refinement.
Production camera code did not change.

## Validation and remaining scope

Binder/KCL tests and all 29 Aurora-native groups pass against the actual Binder.
See tests.md for general contact, storage, flags, aliases, and lifetime cases.
All 59 OnlyCamera/view/stage/event checks pass with the real-disc resource
checks enabled, as does the separate original-camera-runtime target. The
showcase smoke passes; the full Gateway walk still fails its seam-grounding
assertion. See integration-status.md and integration-results.json.
Original-compiler evidence, including 276 reaction cases and 56 zone erasure
cases, is in original-binder-reaction-20260903.

The native prism provider still differs from complete Wii KCollision: it uses
its decoded geometry/BVH and source-prism order, and reports every hit with
face feature code 1 instead of all edge/corner codes. These are explicit
system-level follow-ups; original Binder source alone does not prove complete
collision parity or moving-platform support.

The prior PC grounded-state logic is still incomplete. It equates grounding
with Binder contact and overwrites shadow/ground when that contact is absent.
Original Mario independently performs floor-ray checks at zero stick input;
a lack of Binder planes does not imply airborne. The required animation owner,
floor metadata, slope/landing and writeback closure is described in
mario-grounding-audit.md. No new force or expanded-radius workaround was added
to hide that gap. Full Mario::update and the bunny-to-Rosalina demo remain
unfinished. Full original camera selection also needs the real XanimePlayer
owner, resource loading and Director/catalog closure; see sibling
original-camera-selection-closure-20260903.
