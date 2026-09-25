# Actual ModelManager shared lifetime

Removed `src/compat/ModelManagerOwner.cpp/.hpp`. The actor registry, draw-buffer prototypes, Mario animator construction scope, LiveActor initialization, and scheduler now share the actual `ModelManager` object. The registry retainer is `retain_actor_model`; the wrapper type and former retainer name have no compatibility aliases or remaining source/test references.

`Game/LiveActor/ModelManager` now owns a private native state containing its retained Game heap, two resource-holder tokens, authored animator dependencies, and the initially constructed model/player/core identities. The native factory allocates the actual Game object in the caller's original heap, while the state and shared control block use host allocation. It preserves the existing asynchronous resource construction protocol and unwind-only model mutex recovery; it does not hold the heap-selection mutex across waits.

The destructor restores `mXanimePlayer` to the captured original player before clearing authored animator dependencies, then destroys the captured original core/player/model. This preserves Mario's lower/upper-player replacement and delayed model packet use. Resource tokens and the retained domain outlive this cleanup. The shared deleter retains the domain locally across destruction and operator delete; weak references keep only the host control block, not the scene heap. Existing draw retirement still waits with GXDrawDone before dropping retained prototypes.

Resource tokens are captured before model construction, and original identities are captured immediately after `initModelAndAnimation`, so later visibility/material/display-list initialization failures do not lose completed children. The regular original update/animation/draw methods are unchanged. Raw child arrays keep the existing original arena retirement contract; the shared material-animation buffer remains ResourceHolder-owned.

The targeted native state is created only by the native retained factory, replacing every former wrapper construction. Existing raw `new ModelManager; init(...)` Game paths (MultiSceneActor) keep their former arena lifecycle; this batch does not silently add resource borrowers to unconverted owners. No replacement manager wrapper or global model-lifetime map was added.

The only test edit is mandatory API migration in `OriginalShadowControllerOwnerTests.cpp`: actual ModelManager include/weak_ptr and renamed retainer. Its existing dirty removal of the StageSession include is preserved. No assertions or test cases were added.

All owned paths have before snapshots and hashes in `owned-manifest.json`. `changes.patch` is this lane's exact delta, including preservation of the preexisting shadow fixture edit. No build configuration changes are required beyond the existing source glob dropping the removed pair. No builds, test runs, staging, or commits were performed, per the user's faster-validation instruction.
