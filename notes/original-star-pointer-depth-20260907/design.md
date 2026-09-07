# Original pointer depth ownership

The camera holder's complete controller table retains CameraDPD::reset, which needs the original StarPointerController::mWorldPos. Native StarPointerService currently has no depth/controller data. Returning an invented position is not appropriate.

Retain the original two controller records, their past-input arrays, StarPointerTransformHolder and StarPointerPeekZ in a Game allocation domain owned by RuntimeContext, matching GameSystemObjHolder's process lifetime. Import the original controller TU. Preserve the PeekZ callback body and controller movement body. Adapt only PeekZ construction/submission outside Game: the native renderer already exposes tagged depth snapshots and has no original DrawSyncManager thread/FIFO owner.

Capture each snapshot after the original post-indirect draw cohort and DrawType_0x33, before 2D/image effects. Retain its exact ID, controller positions, view, projection and viewport until completion. A scoped tagged-read selector makes unchanged GXPeekZ calls read that exact completed image. It does not alter the current Game allocation scope. Nested scopes restore the previous selector.

Consume completed captures before camera/player movement after host input publication, matching GameSystemObjHolder::update's pad -> pointer -> scene ordering. Do not consume a pending, dropped, or mismatched image. Retain the actual initialized/previous world point until a real callback completes, as the original controller does when mDrawReady is false. Backend latency can exceed one frame; captured transforms and positions must remain paired with their own depth image. Release all queued snapshot IDs before destroying pointer records.

Existing mouse coordinates are logical framebuffer pixels (640x456), whereas WPadPointer::getPointingPos uses normalized coordinates. The new normalized and historical query providers invert that mapping. Existing distance_to_display state is forwarded unchanged; host mouse input currently uses virtual sensor distance 1 meter while valid. This task does not invent a measured physical distance.
