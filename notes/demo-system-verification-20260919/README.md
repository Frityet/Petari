# Demo system verification, 2026-09-19

**Whole-demo accuracy is not yet established.** The audit found and corrected real collision differences, verified the retained scene setup against the disc, and passed focused original-process tests. The authored scene still has 59 known-unlinked placements, including eight with confirmed model/KCL assets. The final fresh-save run completed all 30,000 frames cleanly, including all three catches, tower activation and Rosalina’s live/unhidden spawn state. Her unobstructed rendered model remains unverified. See [the final run](final2-run.md).

## Corrected source and ownership

Canonical decomp recovery was published first as `4ae0d93c3e9b451da9fc05ec9a31533ce44ae843`, followed by native checkpoint `eda37d29cf809c75dea8d32ab9ab1bb858b2a23c`. Original sphere, point, fast-line/camera and area queries now retain original feature classifications, thickness, ordering/capacity and moving-surface reactions. Triangle vertex positions include translation. Original Binder/updateBinder ownership replaces the extra host motion wrapper; the unused host sphere-response API was removed. See [collision evidence](collision/README.md) and [physics evidence](physics/README.md).

The changes are general collision, native publication and scene ownership fixes. Original source was recovered in `decomp/` before native import; Game changes are original-body restoration, necessary native boundary guards, and debug-only trace reads. No Gateway-specific collision branch, forced catch, switch/nerve write or actor teleport is used.

## Scene setup

The fresh main-process report matches all 243 retained disc row identities, their layers, attached zone transforms and object IDs. It contains 179 supported creators, 59 known-unlinked placements and five metadata rows. These are creator-availability counts, not proof every actor completed initialization. The selected Mario start is root zone 0/MarioNo 0/camera 78. The process probe validates 84 positions, 84 rotations, 48 rail controls, two switch volumes, and four original RestartCube controllers plus real restart-ID dispatch and state restoration. See [scene evidence and omissions](scene/README.md).

The only missing creator in initial MysteriousZone is later-gated SpinGuidanceCube. Missing content on other attached planets includes real model/collision geometry; its visibility and physical relevance cannot be dismissed solely because the intended demo stays on the starting planet. Death/respawn, all camera transitions, every KCL orientation and retail frame-for-frame parity remain outside the passing evidence.

## Validation and limits

- Expanded original sphere/point/fast-line/area query tests pass. `publication-final-query.json` freshly verifies the final source, including all four categories, publication rejection before culling, category isolation, disable/re-enable, repair and retirement while quarantined.
- Original Binder, host storage/registration, area polygon and placement coverage tests: passing entries in `restored.json`. Its initial sphere test compile failure is preserved separately from the later passing `final-query.json`.
- Actual placement-transform process: 120 frames; player-owner: 360 frames with 320 consecutive one-original-movement-per-frame checks; sensor: 120 frames; generated CollisionArea: 360 frames and 12 face mutations/retirement. All pass.
- Actor utility tests: 128 gravity and 27 easing cases, no mismatches.
- Portable factory catalog: two cases execute and pass, two optional disc cases skip. This does not resolve older standalone FileSelect/AreaObj fixture ownership failures.
- Final linked-provider audit: 23,365 strong rows, no duplicate strong providers, stale direct mappings or unavailable inputs. Unreviewed providers and excluded original units remain; this is not semantic whole-game proof.

Initial failed builds/probes are retained, including numeric test initializers, a descriptor order error and fixture ownership corrections. No failure was converted into a pass by weakening the runtime contract. Final publication guards run before original broad-phase culling; their focused test, Binder regression and actual CollisionArea process check pass. A test-source timestamp check caught two assertions added after the first query build, so that exact final test source was rebuilt and rerun in `publication-final-query.json`. Historical instruction reports establish recovered source provenance; this task does not claim fresh MWCC matching or an independent PPC execution comparison.

## Actual route evidence

The unchanged pre-fix baseline completed 30,000 frames but only two catches. The first corrected-source run reached both catches and original post-catch dialogue, with ground/wall/roof contacts and face/edge/vertex features in its sampled trace. It stopped at frame 26970 because a supervised button command used a comma instead of a semicolon. That input error is preserved in [the failed-run record](final-run-input-failure.md), not reported as a game physics crash or a completed demo. Neither of those earlier runs reached the tower gravity transition or Rosalina. The subsequent final2 run did: catches at 3190/4690/10480, plane activation at 11140, Rosalina live/unhidden at 11440, and actual player plane selection at 14440 with post-demo stair grounding. It completed 30,000 frames and exited 0. See [final run evidence and visual limitations](final2-run.md).

The external test operator reads traces and writes ordinary controller input only. It now inverts original stick shaping and keeps full speed while chasing. Its direct-target navigation can stop at physical obstacles; this is distinct from proving a collision defect. Exact input receipts, source/binary hashes, driver revisions and supervised interventions are retained. Actor live/unhidden flags are not pixel proof. Raw traces/images are preserved locally; published gzip files retain identical decompressed bytes and are listed with hashes in artifact manifests.

The initial baseline launch also stalled before frame 0 in an AppKit persistent-window-restoration dialog. Subsequent probes use only process-local `-ApplePersistenceIgnoreState YES`; no macOS preference or production startup path was changed.
