# Mario ground probe and contact recovery — 2026-09-10

The live demo reached original Mario walking and jumping. The parent debugger observed the first idle ground check entering horizontal snap despite grounded=true and mVerticalSpeed=0; the inherited source then moved toward the first perimeter probe by five units. Exact captured input/output evidence is in ../mario-ground-drift-20260910/ground-snap-lldb.log. Root captured that baseline and the integrated native replay below.

## Reference and scope

Read decomp/AGENT_DECOMP_GUIDE.md. This recovery edits only Mario::checkGround in decomp/src/Game/Player/MarioCollision.cpp, then copies exactly that function into the native source. No compatibility shortcut, new physics tuning, or null suppression is introduced. Retail RMGK01 checkGround is 0x802D3A38–0x802D4E70 (5176 bytes); the full original assembly is captured here. The starting function was substantially incomplete, scoring 51.25193% against that exact symbol.

## Recovered behavior

- Simple-ground selection uses the original current movement flags (_14 and _36), selects the shadow-polygon normal except during 崖ふんばり, and clears the correct ground-rejection flag. Normal/rising/status7/statusD vertical limits are 30/10/100/5. Probes lift30 and cast100; their radius50 and rotation120 degrees remain original.
- Four-probe selection uses the correct current-state _23 flag and 壁押し animation. Ground-candidate displacement uses its saved selected point. All-present detection includes the raw fourth hit. Original Fur wall-code literal and original rejection-evaluation order are restored.
- Edge displacement is gated by the original current/prior state and missing fourth-probe/floor-code conditions. Front/back increments are +6/-6. Side wall redirects, status13 side corrections, exact return/continue paths and original trace strings are restored.
- The omitted middle counts same-sensor balance and projected-normal agreement, publishes the original flags and byte, and handles forced grounding plus shadow/ground vertical correction with the original 55,30,1,0.99 thresholds. The unusual original loop bound is i<hitCount, retained exactly.
- A close raw center probe can rescue zero perimeter hits. This uses the local rejection flag; the old code incorrectly consulted an unused lift-rejection slot and returned too early.
- Horizontal snap runs only for (jumping && hitCount!=0) or (grounded && mVerticalSpeed>=5). It measures signed (mGroundPos-mShadowPos) against _368, clamps the existing mShadowPos-mPosition, checks all three wall normals at -0.01, and compares the saved relative distance against verticalLimit. It never uses the sum of world-space hit positions as a distance.
- Shadow and ground fallback projections retain their distinct gating. The original final draw-state return is restored.

## Verification and limitations

Full Wii translation-unit compile passes before and after recovery. checkGround fuzzy comparison improves 51.25193% to 87.42658%; candidate5108 bytes versus retail5176. This remains below the guide's 90–95% aim and is not a claim of byte identity or complete Mario gameplay correctness. The recovered control flow and constants were audited against the complete retail function; depth_completion independently reviewed the probe stage and edge branch and supplied the missing-middle evidence in ../mario-air-gravity-audit-20260910/middle-ground-control-flow.md.

The baseline/fixed compiler commands, logs, objects and detailed objdiff reports are retained. all-retail-floats.json comes directly from the original object symbol/section data; retail-operands.json records exact Shift-JIS string bytes and decoded labels. Native isolated compilation also passes. Root then rebuilt the integrated showcase and ran the real RVZ with SDL key events, exercising the ordinary input and original gameplay path. All **13/13** recorded checks pass: completed960 ticks; finite movement/camera; zero displacement across51 idle samples; W/A/S/D accepted with actual grounded motion and118–127 units displacement; each direction releases; jumps at600 and780 land at635 and815. The run exits0, presenting59.956 FPS with60.083 simulation ticks per second. These are bounded scripted-input results, not a claim of complete Gateway bunny progression or a manual playthrough.

Exact native binary SHA256: `e86ab9830fcb1a53347b8311c9fad4f5e98f302b67c4c28d4838f80b23856017`. Runtime command, real RVZ path, key script, timing and per-check evidence are in native-runtime.json and native-validation.json, recorded by root. The before/after idle evidence is especially direct: the earlier replay moved140.43 units over50 ticks with no input; the recovered run moved0.

Decomp checkpoint `db38bcbac05d051fa9b319306a7ba43cb84f37ae` was authored and committed by `codex <codex@openai.com>`, pushed to origin/pcp-decomp, and verified with ls-remote. Only the exact MarioCollision.cpp path was committed. Root owns the root source/submodule checkpoint and demo packaging. Both decomp and native source file SHA256 values are `cfe09aebb8bea4d6f67292bd2d465f9806621a36323de22135870e00cd44d2e5`.

