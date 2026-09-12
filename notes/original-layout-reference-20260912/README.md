# Original layout/text reference recovery

This checkpoint publishes reference source before native activation. Six changed translation units pass fresh builds with the original PowerPC compiler: LayoutManager, LayoutUtil, MessageUtil, Language, DrawUtil and NPCUtil. source-sha256.json records all nine source/header inputs at publication; original-compiler-results.json records exact isolated commands and exit status.

- Recovered original recursive TextBox initialization and text rectangle measurement, texture lookup/replacement, visibility recursion and text-sized animation frame selection. SDK iterator/Material assertions now follow their retail contracts. Texture operations are above 99% instruction similarity, width/height frame selection 100%, and the rectangle wrapper 95.87%. Recursive helpers have lower similarity from SDK inlining/outlining and are supported by direct retail control-flow review; they are not represented as exact matches.
- Recovered tagged-message visible-character counting and decimal figure counting (95.89% and 100%). Restored Language's original prefix buffer capacity without changing the two-character extraction; its region-prefix function still matches exactly.
- Corrected setupDrawForNW4RLayout's projection limits from retail constants to -1000/+1000. The complete original function compiles at 83.79% instruction similarity.
- Recovered endNPCTalkCamera as the original tail call to endTalkCamera. Retail instruction 0x4bfdb45c at 0x803eeb28 branches to that function; it is not an empty method. The new original compiler output is one branch with the matching relocation.

The compressed evidence includes compiler receipts and reference comparison reports. Raw objects/game assets are excluded. Native layout/Talk/process lifetime integration is still in progress; this checkpoint is not gameplay evidence.
