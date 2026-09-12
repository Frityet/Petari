# Original AlreadyDone owner consolidation — 2026-09-12

Removed the manual AlreadyDone table setup/update algorithm from StageSessionState. The public MR path now delegates through GameDataFunction to the existing original GameDataTemporaryInGalaxy and AlreadyDoneFlagInGalaxy owners. Name hashing, BCSV placement keys, packed values and record lookup have one original implementation.

The scene counter fixture exercises that public path with actual JMapInfo records: masked message-name hash, zone/link distinction, identity after setting the packed value bit, clearing, and all 64 entries. Removed the older test of the deleted native table algorithm. Its invented table-overflow exceptions are no longer part of the compatibility contract.

Validation: the changed translation units and the complete scene counter fixture were compiled and linked in isolation against the existing native archives. The full fixture passes with the real disc's scenario catalog and original profile owner. It was rerun after the upstream reference merge; `result.json` records the exact binary and current source hashes, and `test.log` records the pass. Compilation/link evidence is retained in `build-evidence.tar.gz`.

This independent cleanup is ready while the larger original talk/text import remains in progress. It does not establish dialogue or Gateway progression. EventUtilCompat remains temporarily active and will be deleted when the complete original EventUtil owner graph can be activated.
