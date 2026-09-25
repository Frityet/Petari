# OriginalGameDiagnostics removal

Removed `src/compat/OriginalGameDiagnostics.cpp` and `.hpp`. Their only production consumers were debug checks at the actual `CameraUtil` internal `getCameraContext` and `MarioAccess::getPlayerActor` lookup sites. The checks now live directly in those callers under `!defined(NDEBUG)`, with identical `std::logic_error` diagnostics for absent SceneObjs and absent MarioActor. Release builds retain their original donor return expressions and no extra checks. No generic replacement helper or substitute owner was created.

All four paths were clean before editing. Before copies, hashes, and the isolated patch are stored here. Existing source wildcards require no build changes. No build or test was run, as requested.
