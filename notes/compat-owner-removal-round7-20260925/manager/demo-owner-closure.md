# Actual Demo ownership and dead capture adapter removal

Evidence: root's `gateway-csv-owners-120.log` reaches frame 120 and rejects DemoSheet archive retirement with 14 native borrowers. GrandStar's two earlier actor-parser borrowers no longer appear. Current DemoFunction::registerDemoExecutor and DemoCastGroup::init are complete original functions and do not call the sidecar capture helpers. Searching the entire source tree found capture_executor and capture_cast_group only in their declarations/definitions. Therefore the sidecar's group collection was empty while two actual executors each retained seven sheet parsers.

Ownership is now on actual Game objects:

- DemoExecutor deletes its Time/SubPart/Player/Camera/Action/Wipe/Sound keepers, talk controllers, and original stage-switch controller children.
- Time/SubPart/Player keepers delete their record arrays. ActionKeeper deletes each ActionInfo and its pointer array; ActionInfo deletes cloned functors and its three storage arrays. CameraKeeper deletes its array; CameraInfo deletes the generated part-name buffer and actual camera info. Sound/Wipe AssignableArray members retain their existing array cleanup.
- DemoTalkAnimCtrl deletes camera info and NerveExecutor's existing destructor deletes Spine. DemoCastGroup deletes its JMapIdInfo. DemoDirector deletes only its SimpleCastHolder and StartRequestHolder. The latter already owns its proxy NameObj.
- Director's group-holder NameObjs and CastGroup's LiveActorGroup remain with original scene object registration. These are borrowed containers, not children newly claimed by this migration.
- Partial keeper constructors now release partially initialized arrays; ActionInfo pointers are value-initialized before potentially throwing allocations. Director nulls published child fields before destroying its request proxy during unwind or normal destruction.

The entire compat/DemoDirectorOwnership cpp/hpp is deleted. Actual DemoDirector, DemoExecutor, DemoSimpleCastHolder and DemoStartRequestHolder expose native borrowed-reference retirement methods. Existing ActorRuntimeRegistry iteration invokes those directly on registered original objects; no parallel executor table, capture call, generation snapshot or deferred reclamation remains. The SimpleCastHolder diagnostic counts actual original containers for the two existing integration tests.

No new source file or build flag is required. Remove compat/DemoDirectorOwnership.cpp from Game's source list. Root owns SceneObj binding hook deletion and the CenterScreenBlur/Sphere test call-site replacements. Runtime validation after this final closure belongs to root. No new lease clearing or holder deduplication was introduced.
