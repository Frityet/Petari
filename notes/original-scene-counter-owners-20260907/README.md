# Original temporary and scene counter owners

StageSessionState now retains the real GameDataTemporaryInGalaxy object, AlreadyDoneFlagInGalaxy and JMapIdInfo restart child through typed destruction, with the original Game allocation domain retained until all children retire. Native already-done queries access the same original 64-entry packed records. The duplicated native flag array and restart value are removed.

Original ScenePlayingResult is imported unchanged and created in both host scenes before original HUD initialization. Counter queries read the actual scene/profile/temporary owners. The GameDataTemporaryInGalaxy reset key is retail SceneUtil's constant (0, 0), while the selected scene-entry start ID remains independent; this corrects the earlier bootstrap draft.

The original-scene-counter-owner test builds and runs successfully. It checks Game-heap provenance, domain lifetime, shared original/native flag updates, capacity, Star Bit saturation, life/profile forwarding, original coin query limits, nested sessions and actual host scene phases. Original coin increment/event-Power-Star callbacks and full GameSequence handoff remain outside this checkpoint. StageSessionState currently owns per-stage temporary data; no full scene-sequence lifecycle claim is made.
