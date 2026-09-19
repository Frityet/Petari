# Canonical Player, Screen and System merge

This lane resolves 37 conflicted canonical files: 17 Player, 11 Screen and 9 System paths. The authorized adjacent `include/Game/Screen/MessageEditorMessageTag.hpp` reconciliation supplies the two upstream tag accessors without introducing a second class. No native source was copied, no build or index operation was performed by this lane, and the pre-existing `NPCUtil.d` was untouched.

`merge_player_system.py` records explicit file and whole-method choices from Git stages 2 and 3; `merge_player_system.json` records both input hashes and the resulting file hashes. The script is an audit/replay aid only while the original merge index is present. It must not be rerun after later merge repairs without reviewing those repairs. `merge-player-system-coverage.json` compares scoped method names in each of the 30 conflicted CPP units. This is a source coverage check, not semantic or binary proof.

## Player decisions

The merge preserves whole local recovered methods for Mario direction/correction/angle matrices, input/posture/gravity, actor base matrix, attack/trample/item/cylinder sensors, begin/end rush, spin drawing, animator update, fire-run update and the Warp lifecycle. Using complete bodies avoids accidentally mixing upstream renamed locals or reordered arithmetic into runtime-exercised recovery and input implementations. Upstream surrounding declarations and methods remain. The exact method list is in the JSON report.

`MarioRabbit.cpp` retains the local recovered source because the merged header still uses its descriptive fields, while the incoming CPP uses anonymous offset names; the incoming side contributes no additional method. The other reviewed Player files retain upstream equivalent/reordered bodies, including removal of duplicate unrelated MarioActor nerve definitions in MarioBlown and MarioDamageParalyze. MarioEffect retains both effect tables and full local method coverage with upstream flag/type names; it relies on the merged MovingFollowMtx and Color8 initialization.

The scoped CPP coverage check reports these deliberate moves into merged headers, not removed behavior:

- `XanimeCore::getJointTransform`: `include/Game/Animation/XanimeCore.hpp:102`, same null-guarded transform-list access.
- `MarioSwim::getBlurOffset`: `include/Game/Player/MarioSwim.hpp:35`, same field return.
- `MarioTeresa::notice` and `keep`: `include/Game/Player/MarioTeresa.hpp:14`, same true/update bodies.
- The five LayoutHolder resource accessor overloads: `include/Game/System/LayoutHolder.hpp:16`, same resource table calls/count.

## Screen and System decisions

LayoutManager combines the incoming original lifecycle/archive/draw/pane/group/indirect-texture bodies with the locally recovered animation binding/unbinding methods and the nonempty locale pruning method. The incoming empty `removeUnnecessaryPanes` was not accepted. Pane metadata keeps its local pointer types and subtree-count meaning under the incoming manager field names. `createAndAddGroupCtrl` retains its correct `LayoutGroupCtrl*` result instead of the incoming `LayoutPaneCtrl*` declaration. The first parent MWCC pass exposed a missing HashUtil include for retained `getAnimTransform`; this lane added the actual declaring header and froze again.

LayoutGroupCtrl uses the incoming owner field name but retains the locally recovered `getPane` iterator, omitted upstream. CustomTagProcessor, ReplaceTagProcessor and LayoutCoreUtil share the incoming descriptive fields and table declarations. ReplaceTagFunction gains the upstream bounded variadic wrapper. MessageTagSkipTagProcessor keeps the separate recovered MessageEditorMessageTag header and existing u32 parameter-length contract; that single class gains the incoming inline `getGroup` and `getTag` accessors.

LayoutHolder uses the actual merged `JKRArcFinder*` archive contract and header-owned resource accessors. ResourceHolderManager retains the two explicit local member-function functors used to dispatch creation to the main thread. Overwrite preserves the locally recovered JKRAram constructor and JKRAramPiece DMA override, both absent upstream. WPadHVSwing accepts descriptive field names but retains complete original initialization: the incoming `pPad = pPad` self-assignment would leave the member uninitialized. Audio wrapper, sequence progress and save-file accessor conflicts preserve their complete original behavior with coherent incoming names/types.

## Next native reuse candidates and limits

The largest useful incoming addition is LayoutManager: constructor, movement, calcAnim, draw, pane-controller creation/lookups, archive mounting, draw-info initialization, group-list allocation and indirect-texture replacement. Together with retained locale/binding code, these original methods can replace substantial native layout ownership behavior after the platform contracts are verified.

The canonical CPP still has no bodies for `addGroupCtrl`, `createAndAddGroupCtrl`, `getIndexOfGroupCtrl`, pane matrix-reference creation/lookup, the two pointing overloads, `calcAnimWithoutLocationAdjust`, `getGroup` or `animateRecursive`. These remain real closure work; the upstream merge does not make the whole manager a complete replacement.

Two incoming source patterns require an explicit native boundary review before direct copying. In the LayoutManager constructor, `pLayoutName` is assigned into an array declared inside the `if (a2)` block and used after that block; its C++ lifetime ends before those uses. `replaceIndDummyTexture` infers a resource range using unrelated pointer comparisons and an all-ones sentinel. Their Wii stack/address assumptions must not silently become native ownership policy. MarioEffect also reads high bytes through host-order union members and stores table pointers through u32 hash entries, so its incoming source is not a portable native replacement as written. These are identified import constraints, not claims that the current native implementation has those defects.

## Validation state

Owned files have zero conflict markers and the scoped `git diff --check` passed. All local scoped methods are either retained in the CPP or accounted for by the header moves above. The parent owns the complete conflicted-TU MWCC batch and merge publication; successful compilation or matching must be read from that batch's final receipt. This note does not claim a new runtime or full retail match.

The parent second merged batch (`merged-second/results.json`) compiled all 106 originally conflicted translation units successfully, including every owned conflicted CPP. The first batch errors are preserved; the parent subsequently started a broader scan of all changed units. This 106-unit result is compilation closure only.
