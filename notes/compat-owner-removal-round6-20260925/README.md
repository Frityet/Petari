# Compat removal: lighting, original overrides, matrices, and NW4R

This batch removes 16 more compat files. Of the initial 330 files, 162 are removed and 168 remain. Complete removal is still the priority and is not finished.

## Changes

- Restore the original lighting data, zone selection, light queries and actor-light control in their Game owners. Delete four compat providers and the separate StageLightData/StageLightSceneBinding cache. Original resource holders and stage data now supply the catalog. Native destructors and two-way borrowed registration preserve retirement from either owner. The lighting source audit accounts for 61 complete donor definitions. See lighting/README.md for the necessary native lifetime changes.
- Restore all 77 original MtxUtil functions, replacing four partial providers. The current donor corrects rotation signs, axis selection and relative composition, table quantization, and the original epsilon. Saved retail disassembly supports these corrections and disabling implicit FP contraction. Four degree-to-short conversions use Aurora's defined PPC conversion semantics. No alternative vector-setter provider or invalid-order fallback is retained.
- Restore the 16 original Game JPA/shape overrides in Game/System/Overwrite.cpp and remove four compat files. The game-specific particle behavior belongs here, including the original predicates and StripeX fallback. Other donor Overwrite methods already owned by native SDK implementations are inventoried individually; this is not a claim that the entire donor translation unit is compiled.
- Consolidate Font, ResFont, Pane and native diagnostic handling in their NW4R source owners. Restore the complete original pane methods with existing native binding hooks and the original matrix-copy helper. Move the unchanged PPCSync fence into Aurora's OS library. Four compat providers disappear. Native BRFNT parsing and layout resource lifetimes remain; original in-place font-pointer relocation is not newly implemented.

## Validation

The integrated Xmake application builds. A fresh-save Metal run completes 600 original GameSystem frames, retains the same binary hash throughout, and retires normally. The inspected frame-240 screenshot shows Mario during the waking sequence. This is bounded opening regression evidence; the later Rosalina encounter and complete galaxy are not established.

Twelve focused targets pass, including all primary matrix tests, Xanime core/player, particle resource/manager, J3D geometry, original lighting, Mii font and layout checks. Two additional lighting modes pass, covering player-light replacement/retirement and original GX light submission. Mii font coverage now reads the actual MiiFont/FileInfo archives and executes every existing glyph, metric and rebinding check rather than skipping absent extracted fixtures. The Xanime fixture's duplicate owners of HashSortTable buffers were removed after LLDB identified double deletion; all six assertion groups remain unchanged.

All 21 attempted focused targets compile after fixing the new lighting helper's missing GX include. Nine broader targets still fail; they are not reported as passes. AreaObjCore's runtime math cases pass after the retail-evidenced sign correction, but its strict source-copy assertion was already false for unchanged baseline sources. The effects/image-effects/collision fixtures bootstrap the standalone RuntimeContext without GameSystem and crash in the unchanged language/message path. The area fixture has no original StageDataHolder. The event-camera fixture writes an actor matrix without a model; the three camera suites lack original scene/controller dependencies. Backtraces and exact outputs are retained. These old standalone fixtures require a coherent migration alongside their remaining host-owner removal; no substitute production owners or relaxed provenance checks were added.

The provider audit finds zero duplicate strong providers, zero stale source providers and zero unavailable owner inputs. Its broad approval gate remains red, with 7,632 unreviewed providers and existing unresolved/differing source anchors. Source checks and failure classification are recorded in validation.json and lane notes. This is not a full-suite pass.

## Publication boundaries

The build and tests use the existing working tree, including unrelated pending changes. No clean-checkout or CMake build is claimed. A separate index stages only this batch. Five initially dirty camera/collision fixture files receive minimal HEAD-based lighting setup edits, and tests/xmake.lua receives only two required process dependencies. Other dirty changes and the preexisting staged patch are preserved.

Aurora's required PPCSync commit is published before the root gitlink. The decomp pointer remains unchanged. Snapshot source copies, private temporary save data and bulk provider tables remain local. Full compat removal and the broader Gateway goal remain active.
