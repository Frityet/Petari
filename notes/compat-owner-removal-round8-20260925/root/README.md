# Original source ownership

- Restored complete donor JMapUtil and JointUtil into Game/Util. Deleted JMapUtilCompat, OriginalPlacementJMap and OriginalJointAccess. Removed unused native public generic argument overloads; the complete original private helpers are now authoritative.
- Folded JUTTexture::captureDolTexture into the existing SDK JUTTexture implementation, deleting OriginalJutCapture.
- Enabled the actual CollisionParts and DynamicCollisionObj source files, retaining the native allocation/relocation and diagnostic publication fixes where those objects own the data. Added the already implemented KCollision source at its canonical Game/Map location. Deleted all three Original* compat providers and retained strict floating point flags. These are required native owner fixes, not newly decompiled code.
- Migrated the existing collision-owner fixture to the actual process; retired synthetic nested-scene rollback scaffolding. Existing dirty collision assertions are necessarily adopted with that migration so the removed StageSession/zone APIs are no longer referenced.
- Removed the obsolete stage-zone-only target and wired migrated fixtures to the original application. Test execution is deliberately reduced at the user request; only app build and one short opening smoke are planned.
