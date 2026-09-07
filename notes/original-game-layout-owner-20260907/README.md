# Original GameScene HUD owner source checkpoint — 2026-09-07

This checkpoint restores the complete original GameSceneLayoutHolder and its 21-TU screen/helper graph, registers the real 0x34 scene factory, and relocates the preexisting native LayoutUtil implementation into `src/layout/LayoutUtilCompat.cpp`. The original utility source is restored in Game; parent owns its source exclusion. It does not establish running HUD/gameplay yet.

The holder constructs nine direct owners: MarioMeter, CounterLayoutController, CameraInfo, InformationMessage, OneUpBoard, MissLayout, MarioSubMeter, NoteCounter, YesNoLayout. Meter/counter subgraphs and optional PurpleCoinCounter are included in `cohort.json`. Existing SceneObjHolder transaction adoption captures constructor descendants and retires typed NameObjs before releasing the scene Game arena. No named UI substitute or fake child is introduced.

CounterLayoutController and NoteCounter intentionally have no LayoutManager. Native appear/kill now allow this original state; calcAnim/draw use the original dead/manager/flag predicates. Show/hide update both hidden and off-calc-animation flags. Actual layouts continue to synchronize their real host runtime. Host registry bucket allocation is explicitly outside the Game arena.

Pane follow retains the original control's pointer and mode. Host calculation reflects all four original modes after animation, propagates parent transforms to descendants, and discards earlier child follow transforms when a parent is reflected later, matching the original recursive matrix update order. The native BRLYT renderer remains a 2D matrix pipeline; this does not claim full NW4R 3D pane matrix coverage. Visibility queries read the pane's local flag independently of ancestor or actor life state. The five recovered coordinate/visibility helpers compare 100% against retail. `copyPaneTrans` returns screen coordinates; original LayoutActor::getTrans applies a second conversion, confirmed directly in retail and retained. Position-copy no longer routes a stack-local pointer through the retained follow API.

Fresh validation:

- All 21 original HUD cohort TUs plus LayoutPaneCtrl/LayoutUtil compile with current Wii compiler/headers:23/23. All 23 retail object comparisons complete. Detailed per-symbol scores and exact commands are in `wii-final-proof.json`.
- All 21 native HUD TUs and 3 host layout TUs compile as full LLVM 23 objects:24/24. Factory/test-draft compile evidence is separate; the test draft is deliberately outside this source checkpoint.
- Eleven missing original methods recovered. Six compare 100%; NoteCounter::add 94%, RumbleCalculator::calc 92.42%, derived calcValues 91.15%, reflectFollowPos 96.19%. Typed child matrix recursion compares 64.57%; retail includes two NW4R iterator null assertions absent from the current SDK LinkList inline implementation. The matrix concatenation/copy and recursive traversal were reviewed, but this lower-score method is not claimed to be complete equivalent behavior. Assembly evidence and compact scores are retained.
- SubMeterLayout's integer animation layer `nullptr`→0 plus explicit typed includes compare all 27 emitted symbols 100% against the fresh prior object.
- Reference provenance is current decomp and fresh RMGK01 retail split objects. DOL SHA1: 25c5959534b3c21246c6c7e42021b916b41fb578.

Known next dependencies: original LayoutManager constructor creates a root pane control, which the existing native constructor boundary still omits; this proven initialization step is queued after this frozen checkpoint. Group finder/IgnorePauseNameObj creation, Chip layout forwards, counter/player-state queries and actual pointer guidance/mode/controller owners still need provider closure. The real 0x34 factory now exposes those links. No root build/run or GPU HUD result is claimed. The test draft covers manager-free lifecycle, original rumble, real-resource follow coordinates/order, nine actual owners, and repeated scene retirement; it will enter tests only after the provider closure is ready.

Exact file/SHA manifests: `native-source-manifest.json`, `decomp-source-manifest.json`. Parent coordinates commits and source lists.
