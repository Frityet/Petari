# Original pane effects, 2026-09-13

The production ScenarioSelect layout reached `initEffectKeeper`, which still registered a synthetic effect request through `RuntimeContext` and left `mEffectKeeper` null. The original `PaneEffectKeeper` implementation was already present and linked.

The LayoutActor bridge now constructs and initializes that actual keeper. Its callbacks, emitter arrays and original effect registrations run on the caller's selected Game heap; only the native registry/control block uses host allocation. The supplied `EffectSystem` is authoritative, including the separate real system created by ScenarioSelectScene. Calls without an explicit system use the original `MR::getEffectSystem` route. No substitute effect system is created.

`LayoutActor::kill` again calls the original keeper's `clear`. Native retirement shares the existing LiveActor MultiEmitter cleanup, including older one-shot emitters whose callbacks still borrow a keeper. Keepers retire before the layout's pane and matrix records, or before their managed scene effect system retires. Construction failure after keeper creation also releases that keeper. Explicit process-owned effect systems remain borrowed and must outlive their layout borrowers, as they do in the current process teardown ordering.

The LayoutManager host record now publishes `_78`, the default text/effect group name previously left null. It retains the converted layout basename with the original 63-byte bound and first matching `4x3`, `16x9`, or `Replace` suffix removed. This follows retail constructor instructions 0x80368478–0x803684E4; name backing is host metadata retained until after keeper retirement.

Five missing `MR` layout-effect entry points were recovered in `decomp/src/Game/Util/LayoutUtil.cpp`, then copied unchanged to the native original TU. All five compile to the exact original eight-byte tail calls (100% function matches): emit, delete, force-delete, delete-all, force-delete-all. The two former `RuntimeContext` wrappers were removed from `LayoutUtilCompat.cpp`.

All five relevant native TUs compile (`native-proof.json`); the reference LayoutUtil TU compiles and exact function matches are in `function-proof.json`. No component tests or shared build/run were performed by this agent. Root owns production validation. The manifest includes shared LayoutManagerCompat changes from the coordinated animation agent as well as this effect cohort; starting snapshots are under `before/`.
