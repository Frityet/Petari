# Fresh movement-demo packaging preparation — 2026-09-10

Added only script/package_movement_demo.py. The user's existing untracked script/package_walking_demo.py is untouched; its inspected SHA256 is 60f326015cf765503100cd142f386bde95b24be72ccc6bf94c85b8879fcf1ced. No build was started, no game was launched, and no .app was packaged during this task.

The new script defaults directly to build/macosx/arm64/debug/smg-pc-showcase, rather than a previously staged app. It requires the exact SHA256 selected for runtime validation, checks the native arm64 executable and its signature, derives minimum macOS from Mach-O load commands, and requires system-only dynamic dependencies. The inspected binary declares macOS 26.5 and uses 25 system dylibs/frameworks. This is a preparation-time snapshot, not a claim about future linked binaries or successful gameplay.

Packaging copies only the executable, shell launcher, Info.plist, README, caller-supplied validation note (if provided), and provenance JSON. No RVZ or extracted game asset is copied. The external disc path is shell-quoted; the launcher also accepts SMGPC_DEMO_DISC as an override. Game logs go to ~/Library/Logs/PetariDemo/. Launch defaults are gateway, 1280x720, no frame limit. The default destination is build/playable-demo/Super Mario Galaxy Movement Demo.app, and existing destinations are rejected rather than replaced.

Provenance includes the exact unchanged copied binary hash, packaging script hash, source binary timestamp/size, current root commit, recursive submodule expected/checked-out commits, and status plus hashes for dirty files in src, tests, script, xmake.lua, .gitmodules, aurora and decomp. It explicitly describes this as a packaging-time checkout snapshot, not evidence that these files produced the binary. A validation note is labeled as caller-supplied, not checked by the packager. Both source and copied binary hashes are verified again before the final atomic bundle rename.

Input documentation was checked against src/render/RendererService.cpp:346–403, mouse button handling at 820+, and RuntimeContext.cpp:724+. The README calls these bindings: WASD movement; Space/Enter/left click A/jump/confirm; arrows camera requests; C reset when allowed; Z crouch; X swing/spin when available; mouse pointer; close the window to quit. F9 development free camera and its separate controls are explained without claiming it is the game camera. The caller's validation note is the place to describe which actions actually passed and any remaining limits. Existing Showcase log text mentions Escape, but no Escape handler was found in the active renderer/window input path; the package README therefore documents the verified window-close route.

## Parent invocation after gameplay validation

Use the SHA256 captured for the binary that passed the runtime checks, and a short plain-text note documenting those checks and limits:

```sh
python3 script/package_movement_demo.py \
  --disc '/Users/frityet/Projects/petari/Super Mario Wii - Galaxy Adventure (Korea).rvz' \
  --expected-sha256 REPLACE_WITH_RUNTIME_VALIDATED_BINARY_SHA256 \
  --validation-note /absolute/path/to/runtime-validation.txt
```

Add --dry-run to inspect provenance without creating an app. Supply --output with a new .app path if a prior package already exists. Packaging never rebuilds and never launches the game. A local bundle does not require notarization; distribution signing/notarization is not performed or claimed.

## Preparation evidence

validation.json and dry-run.json record syntax validation, successful read-only inspection of the actual executable/disc checkout, two shell literal-path cases (including spaces, apostrophes, substitution syntax, punctuation and Japanese text), two environment override cases, and rejection of a mismatched binary hash. The generated shell was syntax-checked. No native app launch, full bundle publication or Finder launch has been tested yet, as requested by the parent while runtime validation continues.
