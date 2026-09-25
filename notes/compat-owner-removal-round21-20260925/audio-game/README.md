# Round 21: actual Game and JAU audio owners

Baseline: `4f08c1f69a06d2c5d3483787555434063ff773c3`. This lane deletes six audio compat files and imports the complete existing `JAUSoundObject.cpp` and `JAUSoundAnimator.cpp` donors into their canonical JSystem owners. The existing full Game audio algorithms are enabled by the root build lane; they are not reimplemented in a new service. No decompilation was needed.

## Behavior and ownership

- `AudWrap` obtains the actual wrapper's typed BGM, scene, system-sound and object-holder children. Unsupported AudSystem/rhythm access raises an explicit absent-owner error; there is no fake AudSystem or fallback BGM singleton.
- `AudSoundObject` owns its hash array. Its destructor unregisters through its exact `mNativeHolder` back-reference, then releases its storage. The holder lane clears this back-reference on removal or destruction, avoiding teardown-time global lookups.
- The complete original JAU animation scheduling and Game animation sound cue selection replace the disabled parallel scheduler. `startAnimation` retains the existing native BAS decoding boundary. Sound cue pointers remain full width through JAISound user data and JAISoundHandles lookup.
- Disabled output declines actual BGM, SE and ME starts before absent AudSystem, starter or rhythm objects are accessed. The general JAU layer only checks its real JASGlobalInstance providers; it does not depend on Game policy. Successful sound handles are never fabricated. Normal sound-name resolution and Game object fields remain active.
- Permission state belongs to AudSystemWrapper. Game pause, opening, HOME, error and THP callers use its actual output policy. Volume/limiter output requests do no output work while disabled; no synthetic controller or limiter arrays survive.
- AudSystem's PC translation-unit branch restores sixteen cheap original field/list methods, including menu-state queries, sound-list queries, microphone fields, volume setters and limited-sound fields. Unsupported reset/menu/DVD/output initialization and chord access remain explicit unavailable-owner operations. No AudSystem is instantiated. Its original non-PC body is retained.

## Native deviations from the donors

The JAU object constructor keeps the previously declared default overload; its process method permits the legitimate null system-sound position. Starts safely decline absent actual starter/SE-manager providers. The animator initializes its primitive scheduler fields and resolves native BAS data. Handle user-data comparisons use `uintptr_t`, matching actual native JAISound storage. Game changes are actual-owner lifetime cleanup, the agreed wrapper lookup, and explicit disabled-output boundaries. AudSingleBgm::getSoundID again uses its attached handle as the original does, removing the earlier cached-ID workaround.

## Validation and limits

Source inspection only; this lane ran no build or tests. The root runs the combined build and short real-game smoke. The six retired APIs have no references in current `src` or `tests`. The two imported JAU files preserve complete donor algorithms with the native boundaries above. Audio output, rhythm/DSP initialization and audible playback remain unsupported by this batch.

`owned-manifest.json` records every before/after hash, `before/` and `after/` preserve exact source bytes, and `lane-only.patch` is the scoped delta. `AudBgmRhythmStrategy.cpp` was inspected and remains unchanged. This lane made no test, build-script, index or commit changes.
