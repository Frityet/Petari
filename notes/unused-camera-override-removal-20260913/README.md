# Remove unused camera override state

The original camera migration left active_camera_system_for_camera_util without callers. Removed this unread thread-local fallback, its scoped setter class and eight remaining setter constructions in two existing fixtures. Actual event camera animation resource registration and original MirrorCamera lookup stay in the shared provider; it now includes its SceneScheduler dependency directly instead of the development RuntimeContext.

No replacement aliases or assertions added. Existing camera service objects, attachment and assertion bodies remain. This is unused-state deletion; no broader camera test pass is claimed.
