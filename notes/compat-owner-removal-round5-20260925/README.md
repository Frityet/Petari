# Compat removal: J3D, language, and Wii settings

This batch removes 14 more files from src/compat: 146 of the initial 330 are removed and 184 remain. Complete removal remains the objective; this checkpoint does not finish it or establish the later Gateway encounters.

The J3D animation, loader, draw-buffer, matrix and GD implementations now belong to five complete canonical SDK source files. Existing native resource bounds, pointer relocation, lifetimes and explicit PPC arithmetic are retained. Original inline XF writers return to the J3DGD header. The draw units compile with implicit FP contraction disabled, matching the donor build. Nine split compat files are deleted.

Language queries now use the complete, unchanged Game/System/Language.cpp and the original GameSystemObjHolder field. The alternate language publisher and override header are deleted. Four fixtures now run against the actual initialized Game process; they no longer publish a second language owner. The old standalone MessageHolderOwnership remains a separate removal task and is not claimed to bootstrap successfully.

Aurora now owns the native console-settings catalog, original SC accessors and encrypted product-setting reader in its OS library. This removes three compat providers, the port's SystemConfigService and port-local SDK header. Both build systems include the sources; no obsolete aliases or forwarding files remain. See sc/README.md and the independent source review for catalog behavior and lifetime details.

## Validation

The integrated Xmake application build and all 16 focused targets pass. The layout fixture initially failed because it treated the standalone preview's archive-path field as a requirement of a mounted game layout. It now checks the original mounted archive, authored resources and Count animation/controller, including the selected stopped frames at three life ratios. No production behavior changed to satisfy this test. The original failure log is retained.

A fresh-save real-disc Metal run completes 600 original GameSystem frames and retires normally. Its binary SHA stays unchanged. The inspected frame-480 screenshot shows Mario and the Luma during the opening scene. This is bounded regression evidence, not proof of Rosalina appearing or galaxy completion. Tests/builds used the existing working tree, including unrelated pending changes; a clean-checkout build and full test suite are not claimed. CMake wiring was reviewed, but only Xmake was built.

The configured provider audit finds zero duplicate strong providers, zero stale providers and zero unavailable owner inputs. Its broad approval gate remains red: 7,768 providers have unreviewed provenance, and existing anchored checks include 11 unresolved and two differing entries. See validation.json for exact results and source-review reports for comparison limits.

## Publication and preserved work

A separate index stages this batch. RuntimeContext files, tests/xmake.lua and the committed version of NameObjFactoryPlacementTests receive only the required settings or build changes; their unrelated working edits are preserved. The original staged patch is verified byte for byte before and after committing. Snapshots, temporary NAND data and bulk provider tables remain local.

Aurora is published before the new root gitlink. Its pointer advances through the previously published GX resize commit edffbd1 to the new settings commit 36b44c4cc5287808a6e830660d3a29f44346bad4. The decomp pointer is unchanged; its upstream merge was completed in an earlier checkpoint.
