# SceneObjHolder real-or-absent migration

## Result

The PC Game copies of `SceneObjHolder.cpp` and `SceneObjHolder.hpp` are now
byte-identical to the regular decompilation sources. The retail source remains
in the PC tree for source fidelity and is excluded from the host target until
the complete retail `newEachObj` dependency closure is available.

The host implementation is now isolated in
`src/compat/SceneObjHolderCompat.cpp`. It implements the retail surface with a
strict real-or-absent policy:

- `MR::getSceneObjHolder()` returns the holder bound to the active scene, or
  `nullptr` when no scene owns one.
- There is no function-local/static fallback holder and no compatibility
  setter in the Game header.
- `SceneObjHolder::getObj()` only returns an object that was explicitly
  created; lookup no longer creates an object as a side effect.
- `MR::createSceneObj()` returns `nullptr` without an active scene, and an
  unavailable factory ID remains absent instead of becoming a substitute.
- Supported IDs construct the real decompiled classes. The compatibility
  factory contains no stage, route, or actor-name aliases.

`SceneObjHolderBinding` is host-side scene ownership wiring. `StageHostScene`
creates one real `SceneObjHolder`, binds it for the scene lifetime, and the
binding owns the real objects produced for that holder. A parallel binding is
rejected rather than replacing the current scene silently. When the scene is
destroyed, the binding is cleared and the next scene starts with an empty
holder.

The stage host explicitly creates the three supported scene-wide objects that
the retail `SceneFunction` initialization path creates before placement:
`StageSwitchContainer`, `SwitchWatcherHolder`, and `SleepControllerHolder`.
Failure to construct any of those real classes is fatal to scene initialization
instead of being hidden by lookup-time creation.

## Retail comparison

Retail obtains its holder through `GameSystemSceneController`; that dependency
closure is not yet compiled in the PC Game target. The external binding is the
generalized host equivalent of that ownership edge. It does not synthesize
content and has no knowledge of File Select, the picturebook, Gateway, stage
names, placement names, or route input.

Source identity:

- `SceneObjHolder.cpp` SHA-256 (root and PC):
  `ea2a2a2dad5fcd4c9a789d68477fe74ec63835fc3f1ece6e824b70895e605823`
- `SceneObjHolder.hpp` SHA-256 (root and PC):
  `8e830cfc37c2fbc37d8f8604c0334257d4d56ad584aacfabcca605540867ee7e`
- `cmp` passed for both pairs.

## Focused coverage

`SceneObjHolderRealOrAbsentTests.cpp` verifies:

1. No active scene means no holder, no creation, and no reported object.
2. A raw/unbound holder cannot become an alternate global scene.
3. Lookup does not create a supported object implicitly.
4. Explicit creation returns the real supported class and is idempotent.
5. An unsupported retail factory ID remains absent.
6. A second simultaneous scene binding is rejected.
7. A later scene cannot inherit the previous scene's objects.

The pre-existing stage-switch native test now binds a scene-owned holder and
explicitly creates the retail switch holder before lookup.

## Verification

See `verification.log`. The focused suite passed 3/3, the aggregate
Aurora-native suite passed 28/28, and the full `smg-pc` target built. The root
integration owner is running the authoritative combined title/File Select/
five-page picturebook/Gateway route after all concurrent real-or-absent
migrations settle; no partial or display-collided route artifact is retained
here as evidence.
