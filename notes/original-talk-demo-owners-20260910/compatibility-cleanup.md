# Shared compatibility and original ownership

The user's requested direction is fewer replacements of game behavior and more shared native support underneath original Game code. `Compat` currently mixes these responsibilities: necessary host allocation/encoding/SDK boundaries, retained ownership for actual original objects, partial implementations of Game utility files, and fail-loud entry points whose original owners are not yet present. These categories should not be confused with a need to reproduce game logic in Aurora.

## Actor broadcasts

The old `SceneScheduler::send_message_to_live_actors` walked a snapshot of scheduled actors, deduplicated registrations and skipped suspended actors. Retail `MR::sendMsgToAllLiveActor` walks the actual `AllLiveActorGroup`, rechecks its current count and each actor's liveness, and skips only dead actors and the excluded actor. Thus an unscheduled, suspended or clipped living actor still receives a broadcast. The difference matters to both demo messages and the bunny collector, without requiring any code specific to either system.

The port now compiles the complete, unchanged `AllLiveActorGroup.cpp`. The scene binding creates that actual group before actors are constructed, and the existing native LiveActor construction boundary registers each actual actor with it. Existing NameObj retirement removes borrowed group pointers. The broadcast provider copies the exact original utility body while the rest of ActorSensorUtil remains under review. The old scheduler API and its `scene_messages` parity-trace field were removed together; that diagnostic no longer describes the active call path.

This does not claim that the current native ClippingDirector has become original. Its ownership remains a separate gap. Nor does it manufacture a MessageSensorHolder for a caller that has not created one.

## Complete RailUtil activation

The complete reference RailUtil compiles against the existing native RailRider and common math providers. Its 113 exported functions cover every one of the old GameRailCompat's 28 exports. The full source is copied unchanged and the 254-line wrapper removed, closing the rail helpers needed by original NPC actions. The original clipping-bound calculation samples the curve; the removed wrapper only bounded its control points and ignored the sampling interval.

`original-utility-provider-audit.json` records object-symbol evidence against the pre-change Game archive. The only RailUtil imports outside that archive are the existing SDK vector functions and libc fmodf. The whole ActorSensorUtil also compiles with its missing reference header supplied, but still needs original shared-group dispatch and the two-sensor distance helper before its complete activation. No substitute was added for those gaps.

## Validation status

Isolated compiler/symbol evidence is a prerequisite, not a runtime result. Build and actual-object test outcomes are recorded in README.md as they complete. The full Gateway chase remains unfinished.
