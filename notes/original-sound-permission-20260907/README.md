# Actual sound permission ownership, 2026-09-07

The native JAudioPlaybackService already owned looping-sound permission and the actual PCM voices, but had no trigger permission or combined query. The missing `MR::isPermitSE` is used by original MarioEffect. This change adds the independent trigger state to the same owner and implements all six original submit/permit entry points plus the combined query against it. An absent RuntimeContext remains an explicit missing owner; no fabricated AudSystem object or telemetry-derived gameplay flag is introduced.

Retail AudSystem exempts sound groups 0 (system) and 13 (HOME menu) from both submission flags. The native allocator now resolves the actual retail ID before applying the corresponding permission, preserving those exceptions. Denied trigger allocation occurs before voice-parameter processing, resource recipe resolution, or mixer allocation. RuntimeContext records no successful system/atmosphere playback event when the backend returns null. The existing actor-event-only service still records explicitly logical requests and never manufactures a positional sound handle.

Scene reset restores both permissions, matching original scene/audio reset behavior. Submitting sounds controls new/renewed allocations; it does not forcibly retire existing one-shot voices.

Fresh original Wii compiler/object proof reports **100%** for all seven SoundUtil permission functions and AudSystem::startSound, startLevelSound, and resumeReset (`retail-proof.json`). All four production/focused-test TUs and the extended existing JAudioPlaybackTests compile natively. No Game source is changed for this compatibility work.

`smg-pc-sound-permission-tests` checks all four permission combinations, independent toggles, system/HOME group exceptions, reset, absence, and the lack of resource/device work for state queries. The existing JAudioPlaybackTests additionally checks real ordinary-sound denial, actual exempt-system playback, every MR submit/permit operation, and absence of false playback events. Root build/run is pending the coordinated camera/collision source freeze; this checkpoint does not claim those runtime checks have passed yet.
