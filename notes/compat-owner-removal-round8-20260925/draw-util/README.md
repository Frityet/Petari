# Round 8 DrawUtil original owner restoration

Restored every body from current `decomp/src/Game/Util/DrawUtil.cpp` to its canonical Game owner. Eight complete compat files are deleted; paths and before snapshots are in `owned-manifest.json`.

The full donor restores the silhouette GX pass, texture-projection matrix, nonfiltered capture and direct material draw. Original texture cache initialization (`true`, 128 KiB) replaces the split provider's zero-initialized flags. Existing clear Z, alpha clear, 2D projection, NW4R setup, screen fill, shadow draw and shadow texture access now share the original source and globals.

Only native differences from the full DrawUtil donor are explicit matrix backing pointers for the host GX `const void*` overload, a complete J3DModelData include, Aurora dolphin/gx.h in place of unavailable fine-grained revolution GX headers, and the existing RuntimeContext preview draw toggles before the original actual GameScene calls. The existing outer command/scene scopes remain untouched; this restore does not shorten those phases or restore J3DSys state between dependent original draw calls. `draw-util-vs-donor.patch` captures the complete differences.

Restored the donor inline six-scalar `J2DOrthoGraph::setOrtho` overload so the original Game constructor compiles unchanged. The two methods from `OriginalSceneDrawUtil.cpp` belong to `Game/MapObj/SpinDriverPathDrawer.cpp` and `SpiderThread.cpp`; their exact currently used donor methods are now at those paths. This does not claim either complete actor implementation was imported.

No builds, tests, Git operations or build-file edits were run. The existing recursive Game source glob includes the canonical files; root owns regeneration and integration.
