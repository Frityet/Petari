# Demo, effect, NPC, scene and NameObj merge decisions

Resolved 35 assigned conflicts after reading both index stages and every conflicting hunk. Read `decomp/CONTRIBUTING.md`; formatted the assigned files with the bundled LLVM clang-format. No staging, canonical compilation, native build or GPU execution was performed by this lane. `merge-scene-demo-source.json` records each file, decision and both input/resolved hashes. The Git index intentionally remains unmerged until the parent stages the reviewed result.

## Original recoveries retained

* ParticleDrawExecutor: upstream still contains only the include. Retained the entire recovered constructor, seven draw callbacks, view-matrix variants, draw-state setup and adaptor registration. No rendering body was discarded.
* DemoSoundKeeper: retained the raw `u8` ReturnBgm member and the endian-safe s32 read followed by `u32(value) >> 24`. Upstream's s32 store through a bool pointer would lose the proven leading-byte contract on little-endian hosts and also aliases the byte/padding. Earlier exact retail byte evidence is in `notes/original-talk-demo-owners-20260910/demo/README.md`; upstream `addInfo` and parser naming are integrated. The known out-of-line Sound/Wipe push_back instantiations remain.
* AutoEffectInfo: retained the local constructor's final zero primary/environment colors and lack of offset initialization. Direct RMGK01 bytes at `0x800C4ED4` confirm the final primary-color overwrite to zero. Upstream's constructor instead leaves primary color at -1 and writes three zero offsets. Upstream flag names, draw-order helper and channel-based color parser are integrated, with the standard strtoul declaration retained.
* Retained the locally recovered DemoRabbit and ScenarioSelectScene destructors absent upstream, plus nonconflicting DemoSheetKeeper and ring-buffer declarations. No matching status was increased.

## Upstream improvements integrated

DemoStartInfo now uses upstream named executor/type fields consistently through the holder and request utility. DemoTimeKeeper's equivalent lookup helper and Wipe/Sound `addInfo` methods are retained. Effect group/holder and synchronized-animation records use upstream explicit pointer/count/capacity fields with matching implementations; ParticleCalcExecutor and SyncBckEffectChecker use upstream descriptive members. AutoEffectInfo's exact upstream flag enum was added to the auto-merged header because the merge otherwise retained the old header without names referenced by both new sources. MultiEmitter's single continuation-field assignment was renamed coherently to `mContinueAnimEnd`.

NameObjCategoryList retains the original swap-last removal through the upstream Vector API; NameObjHolder uses its existing array capacity and pop operation. PlacementInfoOrdered uses upstream typed links/iterators, shape identifiers and comparator methods, preserving the same priority/count Shell sort and placement traversal. StageDataHolder upstream provides every locally recovered public method, including rail/child/general-position access, matrix construction, layer loading and recursive placement; its typed TPos3f interface is kept. Reordered duplicate definitions from conflict auto-merge were not retained. MultiSceneEffectKeeper/NPCFunction/Rosetta changes are equivalent getter/member naming or source organization; no local public function is lost.

## Three concrete DemoRabbit repairs from upstream

The earlier 99.45844% aggregate text match was insufficient evidence for these individual semantics. Direct reads of the RMGK01 DOL and its disassembly support upstream:

| Site | Retail evidence | Selected behavior |
| --- | --- | --- |
| updateJump | `0x802713AC` is `cror eq,gt,eq`, followed by a branch that skips the jump if false | Wall hit power `>= 0`, correcting local `<= 0` |
| exeGoal | `0x80271768` loads base `0x805AF6C8 + 0x111`; bytes at `0x805AF7D9` are `Wait\0` | Start `Wait`, correcting local `Change` |
| updateStopVelocity | `0x80271070–80` loads f1=0, f3=1, copies f1 to f2, then calls rebound | Parameters `(0, 0, 1)`, correcting local `(1, 1, 0)` |

`demo-effect-retail-merge-evidence.json` records exact DOL hash, addresses and bytes with checked assertions. Upstream constants and the existing quaternion Z-direction helper are retained. The upstream header's air-timer comment was corrected to the real offset 0x168.

`demo-rabbit-native-retail-fixes.patch` is a precise three-line patch against the existing native TU; the parent subsequently applied it to the native TU. `git apply --check` passes and `demo-rabbit-native-patch-check.json` records source/proposed hashes. It keeps existing CP932 wrapping and all other native source untouched. The native startup receipt now covers the applied patch.

Existing `OriginalNpcOrientationTests.cpp` invokes actual DemoRabbit owners to check orientation caching; it does not assert these wall-jump, goal-animation or rebound branches. Search found no dedicated test asserting the incorrect expressions or `Change` goal animation. That existing test and ordinary guide movement remain useful regression coverage, but a direct owner test for these three repaired semantics is still a separate validation task; no passing runtime claim is made here.
