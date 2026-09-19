# Root audit: enforcement and ownership boundary

Review input: root `01f8c899fc33e7c9e7953d3dff9bc8902895173c`, decomp `d97d7e19a5d80e85c8d73327718c050a2b9e1c70`, Aurora `66c38d52b04427302030ad6d3443efaa2265310f`. Existing unrelated working-tree changes are preserved. This is a source/call-path review, not another gameplay replay.

## P2 — the source-closeness audit misses the actual compatibility directory

`scripts/source_closeness_audit.lua:469-476` inventories camera, layout, selected render, resource, runtime, and scene; it omits `src/compat` completely. `src/Game/xmake.lua:100` nevertheless includes all compat translation units in the normal game archive. The current audit command succeeds and reports 213 compatibility files, but excludes 337 source/header files in `src/compat` (245 C++ translation units). Its Game pass at lines482-519 only checks files retained under `src/Game`; an excluded canonical file can appear source-correct while its linked replacement changes behavior elsewhere. The directory inventory at lines525-539 assigns descriptions from directory names; it does not verify those descriptions against function bodies.

Impact: this report cannot enforce the requested rule or detect moving a gameplay algorithm outside Game. Its 1,690 Game-file classifications (1,162 exact, 244 compile-only, 277 temporary, 7 without an implementation counterpart) are textual classifications, including headers, not counts of hacks or evidence of runtime parity.

Recommendation: include compat and Aurora in boundary inventory, map each overridden Game symbol to its canonical owner and actual linked provider, and require a concrete native constraint for noncanonical bodies. Compare relocated original bodies too. Do not fix this merely by renaming or moving files. Keep SDK/runtime adaptation, imported original engine/game functions, diagnostics, and showcase scripts separately identifiable.

Validation: `xmake source-closeness-audit --output=notes/compat-boundary-review-20260919/source-audit` exited0; generated outputs retained. The first invocation used a space after --output and Xmake rejected that CLI spelling; no source change was needed.

## Ownership debt is distinct from a gameplay workaround

`src/compat/MarioStateCompat.cpp:4-9` explicitly says the original state lifecycle was split out for optional static-link dependency closure. `src/Game/xmake.lua:20-22` still excludes the original MarioState.cpp. Its lifecycle implementation is game logic, regardless of location, but the documented canonical copy is not evidence of an invented state machine. Revisit whether the old link constraint still exists before consolidating it; no link/build proof was attempted in this review.

Similarly, `src/compat/OriginalMapQueries.cpp:52-62` is the canonical stack-based CollisionPartsFilterActor query recently recovered in decomp. It filters any supplied actor; it does not identify CrystalCage or change the stage to make a cage work. The newly restored ModelObj initialization and JMap transformations also remove divergent behavior instead of adding content exceptions.

## Rejected suspected findings

- `GameDataFunctionCompat.cpp:250-260` selecting the last character of the save name with fallback1 reproduces `decomp/src/Game/System/GameDataFunction.cpp:238-247`; this is original save behavior, not a new workaround.
- `GameDataFunctionCompat.cpp:291-295` skipping an already passed story event reproduces canonical lines190-196.
- Immediate gravity calculation in onCalcGravity is itself original (`decomp/src/Game/Util/LiveActorUtil.cpp:2687-2693`). The native numerical helper still warrants review; immediacy alone is not a defect.
- The explicit original Mario model-name table is not automatically a hack. Asset/factory tables naturally contain actor names and should be checked against donor data, not banned by a text search.
- Notes-only replay tools issue ordinary controller input and observe state. They must not be counted as production progression implementations.

## Intended boundary

Original Game owns actor states, event gates, talk flow, scene/demo sequencing, gameplay math, authored factory policy, and resource choices. JSystem/SDK compatibility may decode native records, adapt endianness/pointer width and allocation, and preserve SDK contracts. Aurora owns reusable GX/VI/OS/input/device semantics without Galaxy actor names. A copied original Game function in compat is an imported original provider, not newly generalized compatibility; preserve its original semantics and consolidate its source owner where feasible. Host lifetime wrappers can be class-specific where a real allocation graph requires it, but must not choose gameplay outcomes.

Provenance spot-check: `git blame` dates the GX copy-alpha repair to Aurora commit `05495810` (pipeline-shape update `cf3ffc98`), the native ease-in replacement to parent `0d240156c2`, and the host gravity normalization to `20e99142fb`. These are retained older paths, not behavior added by this review.
