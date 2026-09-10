# User movement preview, 10 September 2026

User requested immediate access to the current build to report issues. Packaged and launched `build/playable-demo/Super Mario Galaxy Movement Demo.app`, binary SHA256 `05028c3a289d505f68c885ff711b35de5b67f3e6e4a69ea76be40325ca3dd753`. Package consumes the existing local RVZ; no game assets bundled.

Compatibility changes: native texture/layout/bright-capture metadata stay outside caller Game heaps; idle original layout frame controllers accept original rate/frame operations; camera publication updates actual J3DSys view state used by original models. No Game source changed in this checkpoint.

Validation: combined showcase build0; real RVZ run presented Mario and Gateway planet using original CameraDirector view through tick90. Screenshot `user-preview.png` inspected. Original layout heap fixture passed, including32 ownership cycles and3 actual FlyMeter GPU frames. Focused texture/bright/animation fixture results recorded separately when completed.

Limits: no claim of verified keyboard movement, user-triggered jump/landing, or camera fidelity. Tick90 capture succeeded but shutdown still aborted(-6), after native device teardown. This is distinct from the repaired retained bright-capture metadata heap fault. Full Gateway bunny chase not complete. Audio/HUD polish out of scope.

Source hashes in preview-source-manifest.json; precise run command/binary hash/exit in user-preview.json. Packaging details retained inside app Contents/Resources/provenance.json. Launch log `/Users/frityet/Library/Logs/PetariDemo/movement-20260910-132643-70360.log`.
