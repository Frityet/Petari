# Concrete prefix-removal include follow-up

The integrated build exposed ScenarioStarter.hpp declaring TQuat4f without its defining header. Root fixes that file and DemoExecutor.cpp. This lane adds the exact JGeometry/TQuat.hpp include to other Game headers spelling quaternion types without a direct defining include. This removes reliance on the deleted prefix's unrelated DirectDraw include and preserves every declaration/body. All before states and include-only deltas are recorded here, including prior round20 literal include changes.

The narrow LiveActor borrow-retirement review found DemoSimpleCastHolder, DemoStartRequestHolder, BaseMatrixFollowTargetHolder and TalkDirector already include the complete actor definition. DemoDirector compares its NameObj and complete DemoExecutor pointers and needs no extra actor include. Explicit native casts in DemoStartRequestUtil already include both LiveActor/LayoutActor. A source/include-chain pass found no additional direct member dereference through a locally declared LiveActor pointer lacking its definition. No speculative includes were added there.

No build or tests were run. Files frozen after the header-only correction.

Concrete source-only quaternion users also gain the defining include when absent from their current include chain: src/Game/Camera/CamPoseSphereInterpolator.cpp, src/Game/Enemy/BegomanFunction.cpp.
