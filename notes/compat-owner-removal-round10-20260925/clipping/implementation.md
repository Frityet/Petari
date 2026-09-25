# Native clipping ownership

Removed ClippingDirectorOwnership and all scene capture/rollback/reclaim hooks. The actual SceneObj factory now creates ClippingDirector directly. Native destructors reclaim the four mutually exclusive actor-info lists, each info's JMapIdInfo, view-group arrays, and group-owned arrays/IDs. Registered child NameObjs remain separately scene-owned.

ClippingInfoGroup records its actual holder and generation and removes its borrowed entry on early deletion. Holder deletion clears each group's owner pointer before freeing the borrowed pointer array. Existing actor-registry retirement notification now originates in the actual holder destructor. ViewGroupCtrl clears its borrowed LOD flag pointers before freeing storage. Constructor-local smart pointers protect partial allocation; the original runtime data structures and clipping algorithms remain unchanged.

No new tests or focused test executions. Root app build and one 120-frame opening run cover this batch under the requested reduced-validation policy.

Source review found LOD publication precedes later throwing model initialization. LodCtrl now unregisters its actual borrowed view entry in its destructor, including failure paths; ViewGroupCtrl detaches surviving LODs and resets their flags to the original false defaults. The redundant actor-registry LOD array cleanup is removed.
