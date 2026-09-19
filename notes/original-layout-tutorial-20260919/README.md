# Original layout locale selection and animated pane dimensions

The original frame2300 tutorial had overlapping copies of its A prompt and a small window behind a long message. The baseline screenshot is `../gateway-compat-20260919/fifo-j3d-final-held-a-2600-frame2300.png`. A separate clean3200frame original-process run captured `../gateway-compat-20260919/movement-trace-baseline-3200-layout.txt` at frame2300 before either behavior change below.

## Evidence and original contract

The baseline contains all six visible `ShaText`, `TxtText`, `ShaTextKrKo`, `TxtTextKrKo`, `ShaTextCnSi`, and `TxtTextCnSi` siblings, each with the identical current Korean message. The Chinese pane font width differs from the other variants. Thus these are actual duplicate active owners, not a hypothetical shader/text-shadow issue. The ordinary original language selection must happen before text initialization and animation binding.

Recovered `LayoutManager::removeUnnecessaryPanes` in `decomp/src/Game/Screen/LayoutManager.cpp` from the current retail DOL's function at80369E9C,796bytes. `retail-verification.json` verifies199/199 disassembled instruction words against `decomp/orig/RMGK01/sys/main.dol`. Original `initArc` calls this after NW4R Layout::Build and before pane information/text initialization (retail asm803695A4). Native `compat/OriginalLayoutLocale.cpp` copies the recovered method/helper exactly; there are no Game source edits.

The routine scans each immediate sibling set for recognized four-character language suffixes using original MR language APIs. When that set contains the current language, it keeps those current variants only and removes the suffix. If localized siblings exist but none match, it removes localized variants and retains unsuffixed siblings. Either localized case stops recursion below that sibling set. Only a set with no localized siblings descends recursively. This also means short names, unknown suffixes, and a current variant mixed with unmarked peers must retain the exact original branch behavior.

The recovery compiles with the normal MWCC flags recorded in `wii-command.json`. Objdiff reports74.944725% for this routine: this is a functional recovery, not a90%matching claim. The199instruction original control-flow/call review establishes the branches above; remaining differences include constructor inlining, registers, and stack temporaries. The virtual bitfield table, destructor and isAnythingTrue helper match100%. No assembly was inserted.

Retail NW4R `Pane::RemoveChild` (`decomp/src/nw4r/lyt/lyt_pane.cpp:91`) only erases the intrusive child link and clears the parent. `Group` resource links were already bound by Layout::Build. Therefore native owners retain detached panes until the layout retires, along with their original group references. Root traversal/lookup/render metadata excludes them; no hidden flag or replacement identity is fabricated. Bound original native animations may still act on retained detached panes without affecting the selected renderer pane. Destruction unbinds animation links before transforms retire and clears intrusive child lists before freeing any owners.

The baseline also has `OneLine` at frame326.7 and Text00.x=-163.35, but WinBalloon and ShaBalloon remain80x80. `retail-oneline-channels.json` records the actual BRLAN curves extracted from global `LayoutData/TalkBalloonStretch.arc`: RLPA8 interpolates width from80 atframe0 to590 at512, slope510/512; RLPA9 retains height80. The expected width at326.7 is approximately405.423828. Aurora previously decoded only RLPA0..7. Original NW4R `AnimatePaneSRT` calls `SetSRTElement`; its contiguous order is translation3, rotation3, scale2, width, height. Both channels8/9 are ordinary SDK pane animation, unrelated to any specific tutorial.

## Changes

- Run recovered language pruning on actual native pane owners immediately after resource groups are bound and before first publication.
- Remap reachable render metadata to the selected canonical names and preserve detached native owners for group/animation lifetimes.
- Remove the guessed per-pane suffix filter/environment default and implicit localized-name lookup aliases. A standalone raw resource viewer has no invented language owner.
- Resolve original group control registrations by member name, as retail `addGroupCtrl` does at80368A10–14. A retained unsuffixed group member can legitimately resolve to a selected renamed pane; pointer identity is not interchangeable with this original lookup.
- Preserve every authored `pic1` record, including untextured pictures. The former parser discarded these when texture_name was empty, making the native SDK owner fail on a valid picture. Original `Layout::BuildPaneObj` constructs all picture records; its Picture/material pipeline supports no-texture materials. The existing broad test's material fixture exposed this preexisting restriction.
- Decode RLPA width/height in Aurora. Propagate both through root/pane layers, committed state, native SDK panes, bounds and debug metadata. Retain direct SDK size edits when no active animation overrides the field.

## Validation

- Aurora standalone `brlan_tests`:11/11PASS, including size channel interpolation and missing-pane behavior (`brlan-build.log`, `brlan-tests.log`).
- Final `smg-pc-original-layout-group-tests` normal build: PASS, 4.594 seconds (`native-build-final.log`).
- `--locale-only`: PASS. Sixteen synthetic real BRLYT/RARC/BRLAN cycles cover selection/fallback/recursion, canonical lookup, retained group identities, actual SDK animations bound to detached panes through retirement, root/pane size layering, frozen animation frames, bounds, and registry cleanup (`native-locale-tests.log`).
- Full existing suite without `SMGPC_REAL_DISC`: PASS, including typed panes, untextured material resources, text tags, heap boundaries, matrices and repeated teardown (`native-all-tests.log`). The optional real-disc FlyMeter window was not run in this no-GPU mode.
- Fixture bring-up corrections were confined to tests: call appear before requesting visible bounds; compare interpolated floats with1e-4 tolerance (observed120.000015x88.0000076); preserve a stopped player's final frame; use the retail RLAN version10 for original NW4R animation binding. The SDK correctly rejected the fixture's older version8 header.
- Actual extracted retail archive mode:PASS (`retail-layout-tests.log`), verifying selected Korean metrics/two text owners and both actual window dimensions against the retail OneLine formula.
- Corrected original run `../gateway-compat-20260919/locale-size-held-a-2600` completed2600frames with exit0, verified process gone and no debugger attachment, in203.17seconds. Parent recorded binary SHA prefix05652f4f. The frame2300 screenshot was viewed: one clear A icon/text copy and a correctly expanded white balloon. Its layout dump has exactly two canonical text panes, four retained detached panes, and both window sizes405.424x80, matching the retail keys.
- The bubble still crosses the left screen edge. This is not claimed fixed or confirmed retail parity: its anchorX=-256.215 and halfwidth~152 put the left edge beyond -304. Original TalkBalloonShort::updateBalloon assigns projected actor position directly, and TalkMessageCtrl::updateBalloonPos uses original CameraUtil projection without clamping. The actual rabbit projects near that edge. No unproven clamp or scene-specific coordinate change was added.
- Independent bounded peer review by build_toolchain found no ownership/index lifetime regressions in the reachable remap, detached teardown, or width/height propagation. It did not claim complete material/animation parity.

Extracted disc payloads/compiler objects and large objdiff output stay in ignored `build/original-layout-tutorial-20260919/`, not committed.

## Publication and scope

- Aurora `66c38d52b04427302030ad6d3443efaa2265310f`, pushed to `origin/codex/macos-compat`; exact remote SHA verified.
- Decomp `8e641dd6a1d75cc9d461f784c72d905ed12253be`, pushed to `origin/pcp-decomp`; exact remote SHA verified. Unrelated NPCUtil.d remains untouched.
- Parent compatibility sources/tests/notes await the root agent's curated checkpoint. No `src/Game` edits.
- The verified2600frame run includes locale graph selection and size propagation, but predates the later group-name registration and untextured-picture follow-ups. Those passed the final focused/full tests and need inclusion in the next main relink; their live-run status is not inferred from the earlier screenshot.
