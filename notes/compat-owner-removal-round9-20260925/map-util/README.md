# Full MapUtil owner restoration

Restored the complete current decomp Game/Util/MapUtil.cpp (decomp 1a126cb5da311fedff662f53fb31c5aeaf851408), replacing the excluded older partial owner and deleting GameMapCollisionCompat.cpp, OriginalMapQueries.cpp and OriginalCollisionGeometry.cpp. All owned paths were initially clean and are snapshotted under before/. No HitInfo, CollisionPartsCompat or StageCollisionService edits were made.

Every public function from the three deleted providers exists in the current donor. Restored donor behavior includes actual CollisionDirector::mCode lookup, including authored numeric/string code handling, the Beach sound case in isCodeSand, original sorted line-hit storage, move-limit comparison, ground probes and fall/danger probes missing from the previous partial port. Null triangle fallback/default code tables are removed; the original APIs require their real scene owner and triangle inputs. No replacement registry or collision publication hook was added.

Native deltas are explicit address-taking for PSMTXMultVec's input vector, self-contained standard includes, and the existing area-query validation moved to the public Game owner. The retained 512-prism / 32-point limits protect actual fixed arrays in CollisionParts::createAreaPolygonList[Array] (src/Game/Map/CollisionParts.cpp:534 and :554-555); empty/null/nonfinite checks are preserved. Existing bounded KCL storage/publication checks remain in CollisionParts and CollisionCategorizedKeeper and were not modified. Three existing donor functions missing from its header are now declared: getSoundCodeIndex(const Triangle*), getCameraCodeIndex(const Triangle*), isSoundCodeSand(const Triangle*).

Coordinated with gateway_gap_audit: the restored MapUtil contains isBindedGroundDamageFire and isBindedGroundWater, the final two non-shadow functions in GameActorPhysicsCompat.cpp. That agent owns deletion of the mixed file after moving its shadow methods; this lane never edited it.

Root wiring: remove the Util/MapUtil.cpp exclusion and replace the explicit ../compat/OriginalMapQueries.cpp -ffp-contract=off entry with Util/MapUtil.cpp. No build, test, index or commit operation performed. No non-ASCII donor literals require CP932 conversion. Source mapping confirms all provider names are covered by the donor; runtime validation belongs to root's integrated short smoke.

## Integration dependency repair

Root compilation exposed the stale TriangleFilter owner/header, which lacked the current donor TriangleFilterDangerCode. Snapshotted both initially clean canonical files, then copied the complete donor TriangleFilter.cpp/.hpp byte-for-byte. This adds the original seven danger-code checks required by isFallOrDangerNextMove without changing existing functional/delegator filters. Existing Game wildcard already builds TriangleFilter.cpp; no additional wiring or native adaptations required. No local build/test performed. Source lane frozen again.
