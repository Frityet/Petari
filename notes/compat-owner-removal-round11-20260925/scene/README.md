# Canonical scene execution

Enabled the complete existing Game/Scene/SceneFunction.cpp and removed SceneInitializationCompat, SceneMovementCompat, and the unused SceneConnectionCompat permanent-retirement API/declaration. Category movement, calcAnim, view entry and opaque/translucent buffer calls now use the original GameSystem scene executor and original category arrays. Native JKR allocation and J3D command scopes live at actual NameObjListExecutor callback boundaries. The renderer's camera presentation and player bridge refresh after their original categories.

Three necessary native boundaries remain explicit: effect initialization retains its existing resource/heap owner; execution-list initialization records existing native allocation/lifetime bookkeeping; draw-category dispatch retains pre-draw callback storage through possible callback replacement/retirement. These are not new alternate Game objects. NameObj restoration and owner-local category retirement safety are documented in the nameobj lane. The draw service now reacquires category storage only when its captured executor binding generation survives the callback.

Deleted J3dSystemCompat.cpp/.hpp because there are no remaining consumers; all model/view code already uses actual j3dSys. No replacement API was added.

Removed LayoutHost calls from OriginalSceneSupport in favor of actual LayoutActor resource retirement. SceneScheduler's LayoutRuntime diagnostic snapshot now covers only the remaining standalone preview owner; actual Game actors have no parallel LayoutRuntime state. OriginalProcessTrace's actor-level diagnostics remain the canonical gameplay observations.

No focused tests or broad new fixture work. Root integrates one app build and a short actual-process opening run. Initially dirty RuntimeContext and test/xmake changes are staged by delta; retained HEAD-only legacy draw methods only lose references to the removed pointer bootstrap. An initially dirty camera fixture's HEAD-only obsolete pointer include/init call is removed without absorbing its unrelated working migration.
