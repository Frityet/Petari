# Original Mario effect category encoding and first-jump audit

## Confirmed first-jump frontier

The native Space/Return mapping publishes CORE_PAD_A (src/render/RendererService.cpp:348). MarioActor sets _F1A=3 (:228); its original checkButtonType case3 reads MR::getPlayerTriggerA. Original Mario::checkKeyLock holds input until actor _37C reaches75 (Mario.cpp:391; retail Mario.s 802A9F40 comparison0x4B). A fresh edge after the lock is required: holding Space from startup does not preserve an edge. MarioMove.cpp:122–125 invokes saveLastSafetyTrans, tryJump, and beforeJumping2D.

The first ordinary tryJump effect is playEffect("共通跳躍") (MarioJump.cpp:96, retail MarioJump.s 802E1FA4). Native objects contain UTF-8 literals: pre-change MarioJump.cpp.o has the full name at byte42342 and no Shift-JIS copy; MarioEffect.cpp.o has it at byte19431 and no Shift-JIS copy. Before this change isCommonEffect still compared literal numeric bytes8B A4; isMaterialEffect compared91 AE. The intended first characters are 共 and 属. Therefore normal native literals failed both category predicates and fell through to direct MR::emitEffect. This was also noted in the previous startup audit; the new audit establishes its immediate normal-jump consequence.

The common takeoff row (MarioEffect.cpp:109) is water-only: flags0x00000C00, so initCommonEffect only registers materialIndex1 and uses name+1 as its alias. Correct original playCommonEffect returns null intentionally on a non-water material (lines575–576). Incorrect direct emission of the full raw category name cannot use that alias; EffectKeeper::getEmitter returns null on miss and createEmitter immediately dereferences it. A separate auto-effect alias/hash collision was not ruled out by a live owner inspection, so the crash consequence was a source-backed prediction, not an observed first-jump result.

## Narrow correction and retail proof

Changed only the two category predicates, first in decomp/src/Game/Player/MarioEffect.cpp and then mirrored those bodies into the native file, preserving its existing PowerPC bitfield adaptation. Each predicate uses strncmp(name, single_character_literal, sizeof(single_character_literal)-1) when the execution-encoded literal is not3 bytes including NUL; otherwise it retains the original two Shift-JIS byte checks. This supports the project's two execution encodings, Wii Shift-JIS and native UTF-8. No name-specific exception, emitter fallback, string conversion layer, or gameplay change was added.

A plain fixed-bound loop was tried first, but MWCC retained a runtime loop and literal loads (64 bytes versus retail40,0% objdiff). Literal indexing with a tail loop also retained loads. The final compile-time size branch eliminates all added code in the Wii build. **Both isCommonEffect and isMaterialEffect independently match their40-byte retail functions100%.** Exact compile commands, objdiff invocation and scores are in final.wii.json, final.objdiff.json, and wii-proof.json. The full native MarioEffect translation unit also compiles0 using current root flags (native.json). These are compile/codegen checks; no live gameplay success is claimed.

Retail predicate addresses: MarioEffect.s802DDD14–802DDD38 (common),802DDD3C–802DDD60 (material). Dispatch at802DDDB8 selects playCommonEffect, otherwise material then direct emission. The tests only inspect the first encoded character; arbitrary following text is intentionally accepted, as in retail.

## Other bounded jump-path surfaces

No additional immediate unsupported query was established in ordinary procJump(true). checkWallRising is intentionally empty in retail (MarioJump.s802E5938 is a single blr), not missing decomp code. The unconditional tail checkUnderWaterFull (MarioJump.cpp:368; retail802E2AA4) queries ForbidWaterSearchCube, actual Water areas, then point collision. The newly installed ForbidWaterSearchCube manager/order25/capacity16 and actual Water manager supply those calls. MR::getWaterAreaObj uses original WaterAreaFunction::tryInOceanArea; when the real WaterAreaHolder is absent it returns false through the original existence guard. WaterInfo::isInWater checks its actual object pointers. Collision::checkStrikePointToMap explicitly allows the null HitInfo output used here. This is a bounded source/retail audit, not exhaustive jump validation.

## Runtime plan and test boundary

The existing MarioGatewayWalkTests cannot currently compile because13 unrelated old actor-model accessor calls have been removed. A new actual-actor regression compiled as an isolated helper, but parent requested no dormant helper; the helper was removed and MarioGatewayWalkTests restored byte-for-byte. The failed legacy compile log is retained only as evidence of why this fixture was not run. No test changes remain.

Use the next real initialized demo owner for runtime proof, after the current graphics frontier is cleared. The adjacent lldb-plan.txt lists read-only character checks and the first-jump breakpoint. Verify the ordinary dry actor context before invoking the original common effect. This work adds no fabricated MarioActor or detached effect owner.

## Publication

Decomp commit a565ce2a56444453c20cf030e142b930178132a9, author and committer codex <codex@openai.com>, pushed to origin/pcp-decomp with matching remote SHA. Native source is frozen for the parent's root checkpoint. source-manifest.json records the exact sources and unchanged legacy fixture.
