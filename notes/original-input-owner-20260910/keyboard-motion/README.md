# Keyboard motion through original WPad — 2026-09-10

The new generalized Aurora `WpadShakeGesture` adapter converts a key state into physical KPAD acceleration samples. RuntimeContext retains independent core/extension adapter instances and publishes them through `set_core_acceleration` / `set_sub_acceleration`; it no longer uses the deleted native `set_swing` shortcut. The existing keyboard spin binding is unchanged. The extension instance is currently sampled with false because there is no separate extension shake key. The adapter and Game code share no gesture thresholds, history lengths, or state flags.

## Physical sample contract

The sampler advances once per 60 Hz input frame. Rest is KPAD `{0,0,1}` (upright +Z gravity, one g). Each rising key edge begins the following 12-sample lateral acceleration sequence, in g:

`1, 1.5, 2, 1.5, 1, 0, -1, -1.5, -2, -1.5, -1, 0`

Y remains zero and Z remains one. Duration is 200 ms, lateral peak is 2 g, and discrete lateral impulse sums to zero. Holding the key after this sequence produces only gravity, without restarting. Release does not truncate an in-progress physical sequence; a subsequent rising edge starts a new sequence. The adapter is allocation-free and does not read Game state. These are synthetic keyboard motion samples, not measurements from a physical Wii Remote.

Original `WPadAcceleration` performs its own SDK-to-Game coordinate conversion. Original `WPadHVSwing` compares the resulting history, including its delayed response to a past pulse; no host code forces one Game trigger per key press or clears the detector on key release.

## Validation and scope

- Complete LLVM23 isolated compilation passes for RuntimeContext.cpp, OriginalWPadGestureTests.cpp, and MarioGatewayWalkTests.cpp (`compile-results.json`).
- The pure adapter contract, extracted from the fixture without the Game integration portion, passes under ASan+UBSan (`adapter-result.json`, `adapter-build.log`, `adapter-run.log`). It checks rest, all finite samples, gravity, balanced impulse, peak, bounded held behavior, rearm, and continued motion after release.
- `OriginalWPadGestureTests.cpp` uses the actual WPadOwnership and SDK publication, never writes Game detector flags, and covers: the first 20 warmup records; history age20 becoming available on sample21; actual core swing/trigger; independent FreeStyle extension acceleration/detector; completion while held; delayed-history settling; release/retrigger; and reclamation of two whole owner generations.
- MarioGatewayWalkTests now queries original MR::isCorePadSwing / MR::isCorePadSwingTrigger and verifies actual acceleration-history warmup. It preserves the locked/unlocked Mario entitlement checks. Release expectations wait for the physical pulse and original history to settle instead of treating the key release as a Game-state reset.

The full original WPad owner activation and linked runtime results belong to the parent's shared build. At source freeze this subtask has not run the integration fixture or the Gateway walk test. Requested new target: `smg-pc-original-wpad-gesture-tests`. No Xmake commands or commits were run in this subtask. `source-manifest.json` records the five owned source hashes; all production/test sources are frozen.

## Linked-fixture correction

Parent's first linked run reached only the extension timing assertion. The complete original WPad constructor sets `mSubPadSwing->_8 = 2.0f` (native/reference WPad.cpp line29), so the test's assumption that its first 1 g sample triggered was incorrect. The fixture now asserts that the first 1 g and second 1.5 g samples remain below the extension threshold, and that the third 2 g sample triggers at the original inclusive boundary. Core state must remain idle in all three frames. No production code or pulse parameters changed. The corrected fixture compiles with LLVM23, exit0; its linked rerun belongs to the parent.
