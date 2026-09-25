# Missing utility link bodies

Added eight exact current donor bodies to their already compiled canonical owners. Each file was clean before this scoped edit; the before copies and exact deltas are captured by owned-manifest.json and patches/.

- ActorMovementUtil.cpp: faceToVector(MtxPtr, TVec3f, f32), makeMtxOnMapCollision, calcVelocityRailMoveOnGround.
- LayoutUtil.cpp: getPaneAlpha, setLayoutAlpha, setPaneAlpha, copyLayoutDrawInfoWithAspect.
- MessageUtil.cpp: getMessageLine.

No prior native body was replaced or reformatted, no declarations changed, and no build wiring is needed. Message traversal calls the existing native MessageEditorMessageTag accessors, preserving the widened-wchar representation and retained original UTF-16 tag units. It does not reinterpret host wchar bytes as packed PPC text.

The bodies are copied verbatim from decomp/src/Game/Util. The original message capacity/tag-copy behavior is retained; this batch does not independently redesign it. No builds, tests, commits or index changes were run by this lane. Root owns integrated linking and validation.
