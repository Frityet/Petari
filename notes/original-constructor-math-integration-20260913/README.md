# Original constructor and math integration

Deleted the remaining NPCActorRuntimeCompat file. Its seven math substitutes now use original shared MathUtil/MtxUtil and SDK quaternion routines; original gravity, animation and shadow initialization live with their existing owners. SDK Euler quaternion construction was recovered in decomp AnmPlayer first and copied to native JGeometry. Removed the unread camera-system override state and its fixture setters, preserving the real original camera owners.

Restored complete original Sky, SpaceInner and MirrorReflectionModel and enabled SummerSky's original factory entry. Filled the original hideModelIfShown dependency exposed by the first link. Restored original model/submodel existence queries without RuntimeContext. Water, indirect, bloom and LOD creation now constructs the actual PartsModel/ModelObj children and starts their original animations; catalog enabling is a separate pending step.

LayoutActor now constructs the original StarPointerLayoutTargetKeeper instead of rejecting the already-available NW4R graph. Native retirement releases its original targets and pointer array before the layout storage.

The fifteenth full production build passes. Ninth real-disc Metal run completes GameSystem::init, enters its frame loop and passes the prior pointing-target failure; it then stops on 'SMG runtime context is not active.' The next known call in ScenarioSelectLayout initialization is the obsolete layout effect registration. That original PaneEffectKeeper migration is in progress. No completed Logo, Gateway wakeup, bunny capture or Rosalina appearance is claimed.

No broad test suite or new component tests. Per-cohort reference/compiler evidence remains in task notes. Current working-tree build includes the user's preexisting compile/header changes, which are excluded from the commit along with their staged documentation deletions and other unrelated files. Reference 17bf274b1 is published before this native gitlink.
