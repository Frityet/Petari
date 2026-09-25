# Round 16: remove the collision service

Baseline root `16d91cbb0`. Deleted `src/scene/StageCollisionService.cpp/.hpp` entirely. No replacement service, global registry, surface cache, triangle IDs, or parallel matrix state was added.

Actual CollisionParts retains its bounded decoded/generated allocation and resource lease. Original zone membership is the only enable state. Generated mutation clears a per-part validity flag before validating the typed resource, original server/file identity, fixed prism count, and active prism geometry; only successful validation restores the flag. Nonpositive-height original prisms are deliberately inactive and skip vertex reconstruction. Invalid enabled parts fail the actual keeper's six query entries before spatial culling, by scanning its existing zone arrays. Other categories remain independent. The six direct part query entries also guard their own generated state.

Matrix updates still require finite, invertible actual current/previous matrices. The deleted diagnostic cache's transformed-world-space approximation no longer rejects source geometry merely because that duplicate representation collapses. Static KCL uses the existing bounded native decoder instead of a second raw byte decoder.

Actual actor retirement now releases generated collision arrays immediately. Retained original Triangle instances already use the actual part's weak lifetime, so safety does not depend on retaining a scene diagnostic cache.

OriginalSceneSupport loses its collision member/activation. Coordinated NameObj lane deltas in that same file use NameObjHolder::snapshotNativeObjects, NameObj::detachNativeHolder, and the three-argument SceneExecutionBinding constructor. Its unrelated scene lifecycle remains intact.

Existing fixtures read actual Triangle geometry, part/zone membership and resource provenance. NameObjFactoryPlacement and OriginalProcessCollisionArea now use normal unique_ptr owners in place of the deleted test child capture helper. Player utility/process fixtures resolve the actual CollisionDirector. Removed the service-only StageCollisionRegistrationTests file/target and the synthetic publication_before_culling block from OriginalSphereQueryTests; the original sphere/line/area query cases remain. No new tests or fixtures were created.

Root owns xmake and integrated validation. This lane ran no builds/tests and made no Git/index changes. Root reported its integrated app build and fresh 120-frame smoke passing before these final fixture-only edits. Those fixture edits were not run here.

The 15-path manifest, before/after copies and lane-only.patch preserve scoped deltas. Two fixture snapshots intentionally begin after the coordinated resolver/zone migration from decomp_validation; that lane retains its earlier snapshots. Initial dirty source/test work is preserved in full before snapshots; retiring the service and its synthetic fixture is intentional. source-validation.json records zero retired service/helper references and six guards on each original query owner.
