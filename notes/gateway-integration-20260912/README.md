# Gateway compatibility consolidation integration — 2026-09-12

This checkpoint follows the user's cleanup request: remove specific replacements, reduce duplicated behavior and storage, prefer real original owners, and use narrow explicit native Game corrections only for architecture/compile differences. Details: ../compat-consolidation-20260912/README.md and remaining-frontiers.md.

The preceding root checkpoint 2fa4fcbe fixes cross-worker Scene retirement ownership and was already committed/pushed. This integration contains restored NPC table access, ResourceShare, original ClipArea/Binder filters, original shadow clipping, LayoutHolder/ResourceAccessor ownership and the cleanup wave. Aurora and decomp are published before the parent gitlinks.

## Verified behavior

validation-summary.json selects the final successful focused receipts; raw earlier failed runs are retained to show diagnosis rather than overwritten as successes. Collision passes two complete scene lifetimes, physics passes 8/8, and three selected original Binder/gravity groups pass. Full original message ownership passes two lifetimes: each compares 6 system and 1,994 game messages, 957 message nodes, 542 branch paths and 92 event nodes, with original metadata, aliases and teardown. The five file-select fellow names are independently compared to authored DAT1 words with both source bounds and destination guards. Original ResourceHolder/LayoutHolder, shared buffers, NPC items, layout tags, real fonts and shadow tests also pass.

The unfiltered legacy Aurora-native suite still fails on missing original WPad/GX fixture owners before the relevant Binder tests. Its three affected groups were run separately through a generic test-name filter. The legacy restart fixture fails at an unrelated original MarioHolder query because it supplies a synthetic player. These are recorded limitations, not full-suite passes or production fallbacks.

## Gateway smoke

The latest showcase builds and completes 240 simulation ticks with keyboard forward/jump input, exits 0, and renders a captured frame. Of 242 authored scenario objects, 78 supported placements are created, 8 helpers ignored and 156 placements remain blocked. See smoke.json for the exact binary hash, arguments, timestamps and renderer counts. This debug run displayed about 16.7 presents/second while advancing about 57.8 ticks/second; it is startup/render/input evidence, not a performance acceptance claim.

Gateway screenshot gateway.png was visually inspected. It shows Mario on the planet with the authored environment and HUD. It does not establish a complete bunny chase, Rosalina spawning or original GameScene progression. Those remain the active goal.

## Publication and preserved work

Aurora c7db821cef0240ca5c4b930ba5cdc5cba421e8a5 and decomp 4e19b524de6457bd4d3b9a31e0c51c0884473bf1 were pushed and verified against remote branch heads. The user's pre-existing document deletions, LiveActor header edit, build warning flag, older notes edit and package_walking_demo.py are preserved separately.

The final stage-initialization resource fixture also passes all three real-disc partial-initialization/retirement generations. It owns actual DemoDirector and NameObjGroup before camera actors use them. The new missing-GameScene assertion runs within this real StageSession/SceneObj lifetime. In total: twelve focused suites and three selected native Binder/gravity groups pass.

Each package's evidence-logs.tar.gz preserves its text build/run/debugger logs; compiled object files and other build outputs are deliberately not included.

The exact PNG is stored in gateway.png.gz (lossless byte compression); the uncompressed local gateway.png and smoke.py output preserve the original image.
