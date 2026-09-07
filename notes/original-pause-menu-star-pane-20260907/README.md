# Original PauseMenu star-pane recovery

Recovered only `PauseMenu::updateStarPane` in `decomp/src/Game/Screen/PauseMenu.cpp` and added its real GalaxyStatusAccessor include. The native PauseMenu TU remains unselected pending complete mandatory GameScene child ownership.

The retail function is `updateStarPane__9PauseMenuFv`, 548 bytes at 0x80375B64. Source: `notes/gateway-audit-20260907/restoration/retail/asm/Game/Screen/PauseMenu.s`, with the original object alongside it under `retail/obj/Game/Screen/PauseMenu.o`.

The original pane arrays contain ShaStarA–G and PicStarA–G in that order. All seven shadow panes are hidden before either stage predicate. Ordinary scenarios show each normal-star slot, hiding its picture if unowned. Owned non-normal stars pack after the normal scenarios. Before AstroDome unlock, the loop stops after its first iteration and freezes the Stars/Star layer-1 animation at frame zero. Otherwise the exact animation frame is owned hidden stars + normal scenario count - 1. Repeated accessor calls and stage predicate short-circuit order are retained.

Validation: full reference TU compilation passes; the recovered function matches retail 100% (548/548 bytes). All 28 already-paired code symbols retain their prior scores. The full candidate also compiles to a native object using the existing private original-child/AudSystem declaration overlays, without importing a reduced owner. The initial native probe put the entire decomp include tree before native headers, which incorrectly selected reference JMapInfo and failed; the corrected probe preserves native-header precedence. This is source/compile evidence, not a linked pause-menu or gameplay claim.

`function-proof.json`, `wii-compile-results.json`, `native-compile-results.json`, and `decomp-manifest.json` record the exact evidence. The large intermediate objects and objdiff documents are local proof artifacts, not checkpoint inputs.
