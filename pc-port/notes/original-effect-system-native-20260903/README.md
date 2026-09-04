# Original EffectSystem native construction and calculation closure

Frozen staging; no production source, RuntimeContext, xmake, legacy effect provider, or root source was changed by this task. Parent-owned scheduler trace corrections are separate. This package closes the original manager graph and its direct native prerequisites. It does not activate actor EffectKeeper ownership or particle drawing.

## Evidence and scope

The isolated complete source cohort compiles with LLVM 23 and ASan/UBSan. Compiler flags contain native Game/SDK paths only: there is no root `include` or root JSystem fallback. `native-header-audit.json` records each dependency file; `literal-root-sources.json` records the 22 literal original source/header hashes.

`probe-runtime.log` records two actual-disc CPU scene cycles using the production ParticleResourceOwnership, the real SceneObjHolderBinding, actual EffectSystem, actual AutoEffectGroup data, and actual scheduler/category owner. Each cycle creates 12 NameObjs: the root, seven draw adaptors, three calc adaptors, and one movement adaptor. Categories are 71–77, normal calc 19, pause-ignore calc 20, and movement 20, directly from the original constructors. The root constructor graph uses 3,440 scene-heap bytes. A domain 32 bytes below its measured requirement fails the final 40-byte allocation; the existing factory transaction removes all NameObjs and scheduler entries. The fixture then rolls back callback history and verifies complete root-heap reclamation. It parses all 2,591 actual AutoEffectInfo rows and validates all 170 authored packed color fields against their RGB strings.

The CPU process has no real screen texture owner, so it does not invoke `entry`; that branch remains in the complete linked graph. No texture or effect-service substitutes are supplied.

The parent separately ran the unchanged `runtime-probe.cpp` with the real AuroraWindow/Renderer and RuntimeContext. The final passing log is `../scheduler-trace-heap-lifetime-20260903/runtime-effect-proof.log`. Two actual RuntimeContext cycles create the process particle owner and capture texture, call original `entry(32, 4)`, verify IndDummy stores the exact real screen ResTIMG, create four actual authored emitters in groups 0/1/7/8, execute eight frames through original NameObjAdaptor movement/calc callbacks, verify calc does not advance twice without movement, exhaust/reuse the actual four-slot pool, and retire the complete scene before process resources. Entry plus emitter storage consumes 8,048 scene bytes. Drawing is deliberately not invoked.

The first renderer run exposed scheduler debug strings/vectors allocated in the selected Game heap and retained by RuntimeContext after scene retirement. The original failure log/backtrace remain here as `parent-runtime-probe.log` and `parent-runtime-backtrace.log`. The parent fixed metadata routing in SceneScheduler itself, without enclosing original callbacks in a host scope, and reran the same fixture successfully. The prior 19,208-byte scene allocation included that metadata; it is not the final graph budget. The final log includes renderer shutdown buffer-mapping/device-destruction messages; this package makes no new claim about those renderer diagnostics.

## Frozen patches

Apply from the repository root. Each patch passed `git apply --check` at freeze time; per-file base/final hashes are in the corresponding manifest.

| Patch | Purpose | SHA256 |
| --- | --- | --- |
| `aurora.patch` | Six generic SDK files; independently activatable | `398ecea5425be98e84928448fb8b35b2f22ca3be49ee8b555371a5b164adf4aa` |
| `general-native.patch` | Color8 integer packing and active scheduler routing | `f3b70938e24dade9c897a8ae765864c2144837733ad120e89d552acc57f3a857` |
| `effect-cohort.patch` | 28 original effect/source/header paths, gated activation | `0b32273c2643ffbaf4752df91d4b7ae22bbf9106fc29eeb3fc9c1e8a3314fc50` |
| `integration.patch` | Existing SceneObj factory case and literal original initialization entry point | `a79815fc48053dc00b27a048be4f18777a736d15184f26f7dfb9d84d3c757f1d` |

`native.patch` is an aggregate alternative to `general-native.patch` plus `effect-cohort.patch`; do not apply both alternatives. Its SHA256 is `519abd28e6d6dd9aaee0efcd1af368edd417695fe6ada6da5332541001b7b4de`. General patches are independent of the gated integration patch.

The gateway's `original-multi-emitter-callback-native-20260903` package overlaps the identical AutoEffectInfo and SingleEmitter source/header pairs and MultiEmitter/MultiEmitterCallBack headers. Apply each shared path only once; the cohorts are not alternative implementations.

## Generic SDK changes

`GXSetMisc` is implemented against Aurora's existing actual shadow state. XF_FLUSH stores the original truncated u16 count, clears Aurora's positive pending-BP flag, and marks VCD dirty only for nonzero counts. DL_SAVE_CONTEXT controls the existing actual display-list save/restore implementation. ABORT_WAIT_COPYOUT retains the original boolean setting and GXInit default. The enum preserves tokens 0/1/2 and adds original token 3.

The original SDK uses complementary `vNumNot` and `bpSentNot` halfwords: flush happens only when both are zero. Aurora stores the positive count and positive BP flag, so the equivalent guard is `vNum != 0 && bpSent != 0`. Both GXBegin and GXCallDisplayList now use that explicit guard instead of interpreting two adjacent native halfwords as one integer. Aurora's existing flush-primitive backend behavior remains unchanged. `GXAbortFrame` has no native consumer implementation; storing token 3 does not claim frame-abort support.

The shadow-state abort flag is appended, preserving offsets of every existing field. The isolated proof recompiles GXManage (actual state allocation), GXDispList (whole-state copy), and GXVert (guard). Normal Aurora builds should rebuild normal header dependants.

`verify-original.py` compiles complete, unchanged root GXMisc.c and SceneFunction.cpp with the configured original compiler. `GXSetMisc` at 0x804BBF14/0x8C and `SceneFunction::initEffectSystem` at 0x803450C8/0x58 match every DOL instruction after relocation normalization. DOL SHA1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. The three retained retail disassemblies show GXSetMisc and both flush guards.

The functional shim adds the original `mem_fun_ref` name and a unary negator specialized for Aurora's member-function adapter. It uses the existing std::invoke dispatch so const/noexcept methods and mutable reference identity survive. It does not replace modern standard-library algorithms or add a Game-specific predicate.

`verify-gx.py` passes 42 sanitizer checks for u16 truncation, dirty flags, no-op token 0, nonzero booleans, all flush-flag combinations, and actual display-list context retention. `gx-aurora-only-link.json`/`gx-aurora-only-runtime.log` additionally prove the executable links/runs without any `libsmg-pc-*` archive. `verify-functional.py` tests mutable reference identity, const/noexcept dispatch, and empty/exhausted/partly-valid `std::find_if` ranges.

Exact normal-build test sources and target proposals are in `normal-tests/`. Copy the two `.cpp` files to `pc-port/tests/` and append `targets.lua` there to expose `smg-pc-gx-misc-state-tests` and `smg-pc-legacy-functional-adapters-tests`. The GX source uses the production-relative private-header include; its minimal include/define compile is recorded in `normal-gx-compile.json`. Neither target needs the effect cohort or a disc. During normal-target activation, the parent found that aurora-gx uses the actual MEM1Start/MEM1End globals and added its missing `aurora-os` dependency in Aurora xmake.lua; the earlier broad application dependency graph had supplied that SDK library implicitly. This parent-owned build-graph correction is outside the frozen six-file Aurora patch. The existing `smg-pc-msl-functional-tests` remains applicable.

## Native source/provider inventory

Select these eleven complete original TUs together for this manager closure:

- `Game/Effect/EffectSystem.cpp`
- `Game/Effect/ParticleEmitter.cpp`
- `Game/Effect/ParticleEmitterHolder.cpp`
- `Game/Effect/ParticleDrawExecutor.cpp`
- `Game/Effect/ParticleCalcExecutor.cpp`
- `Game/Effect/AutoEffectGroup.cpp`
- `Game/Effect/AutoEffectGroupHolder.cpp`
- `Game/Effect/AutoEffectInfo.cpp`
- `Game/Effect/SingleEmitter.cpp`
- `Game/Effect/EffectSystemUtil.cpp`
- `Game/NameObj/NameObjAdaptor.cpp`

All direct/transitive providers retained by constructor, entry, emitter creation/deletion, calculation, and the seven actual draw adaptors link in the runtime fixture. The only new SDK link frontier was GXSetMisc. The complete utility TU also contains actor, layout-pane, and MultiScene registration functions; compiling it does not mean those unused receiver graphs were activated or tested. Those unused functions are dead-stripped in the bounded fixture.

The exact additional declaration headers are recorded in `effect-cohort-manifest.json`, including MultiEmitter/CallBack, PaneEffectKeeper, MultiSceneActor/EffectKeeper, and the complete JPADrawInfo one-matrix constructor. Missing headers are explicit staged files, not root fallback paths.

The existing native SceneFunction connect/disconnect bridge now honors SceneSchedulerBinding, matching predraw registration and NameObj destruction. When that selected scheduler is the RuntimeContext scheduler, existing model/layout registration paths remain in use. Color8 integer construction/conversion now maps the original RRGGBBAA numeric value to host-independent component bytes; actual authored AutoEffectInfo data supplies the regression evidence.

Retire `compat/OriginalParticleResourceQueries.cpp` when selecting complete EffectSystemUtil.cpp: its four literal resource queries then have their original provider. Retire gateway `compat/OriginalEffect2D.cpp` at the same time. Keep the production `OriginalParticleResourceLookup.cpp` actual-owner lookup. Gateway's string-query extraction remains necessary until a selected StringUtil provider supplies `hasStringSpace` and `isDigitStringTail`; their presence in an unselected source file does not resolve linking.

`integration.patch` extends the existing SceneObjHolder factory with the literal original EffectSystem case; it adds no parallel owner. It adds the complete original SceneFunction class declarations to the existing native header without replacing native bridge declarations, and supplies the literal complete `SceneFunction::initEffectSystem` body as a temporary extraction. Both integration TUs compile without root fallbacks. Remove that extraction when selecting full original SceneFunction.cpp.

The actual retail startup caller is `GameScene::initEffect` in root GameScene.cpp: it passes the original stage-dependent counts to SceneFunction::initEffectSystem. The fixture's 32/4 counts and 128-KiB test arena are bounded test inputs, not a proposed replacement for that original capacity selection.

## Owner and activation requirements

1. Initialize the actual process particle owner only after archives are ready. Retain it and the existing real screen capture texture for every scene emitter that borrows either.
2. Construct through the existing SceneObjHolderBinding in the selected scene JkrAllocationScope. The exact root is published only after construction/init succeeds. Do not call emitter methods before original entry completes; the original constructor intentionally does not initialize `mEmitterHolder`.
3. Retain the actual scene domain through every NameObj/functor borrower. The SceneObjHolder binding owns the registered descendants in creation order and destroys them in reverse; original NameObjAdaptor destruction deletes all four possible cloned functors. The non-NameObj executor, group, metadata, emitter-holder, and JPA-array graph remains in the actual original arena.
4. On scene failure or retirement, stop/delete active emitters while their SingleEmitter/callback owners still exist. Remove scheduler registrations and roll back predraw history to the scene marker while the exact NameObjs and process resource owners are still live. Then destroy the binding/actual NameObjs and functor clones, release the scene domain, and finally release process resources. The fixtures prove this ordering and complete heap reclamation. Entry-allocation failure itself has not been separately injected.
5. SceneObjHolderBinding currently rolls back NameObj factory registrations; it does not independently own a scheduler predraw marker. Its containing scene lifetime must perform that rollback, as the fixtures do, including constructor failure. Do not release the arena while retained predraw-domain leases remain. Parent-owned SceneScheduler trace allocation fix is also required for invoking movement/calc in a Game scope.
6. Before activation, retire the legacy EffectService draw submission at the end of SceneScheduler::execute_draw_type and its actor-facing effect facade atomically with actual EffectSystem/EffectKeeper ownership. No overlap was enabled here. The true actor/layout/MultiScene owner and metadata-registration paths are subsequent work; no fabricated emitter, texture, or null-return Game provider was introduced.

No missing root method remains in the bounded manager constructor/entry/calculation/draw-registration graph. Original actor EffectKeeper/MultiEmitter ownership, remaining active receiver providers, and actual particle drawing are the next activation boundary. This task supplies neither a drawing-success claim nor full gameplay completion.

## Reproduction

From the repository root, with the existing original compiler tools and current native dependency archives:

```sh
python3 pc-port/notes/original-effect-system-native-20260903/verify-original.py
python3 pc-port/notes/original-effect-system-native-20260903/stage.py
python3 pc-port/notes/original-effect-system-native-20260903/compile-gx.py
python3 pc-port/notes/original-effect-system-native-20260903/compile.py
python3 pc-port/notes/original-effect-system-native-20260903/link.py
python3 pc-port/notes/original-effect-system-native-20260903/verify-gx.py
python3 pc-port/notes/original-effect-system-native-20260903/verify-functional.py
python3 pc-port/notes/original-effect-system-native-20260903/link-runtime.py
```

`link.py` runs only the CPU actual-disc fixture. `link-runtime.py` only compiles/links; running its binary creates an actual renderer and is parent-owned. Sanitizers cover the newly compiled cohort and SDK providers; prebuilt dependency archives are not retroactively instrumented. `freeze.py` is an audit/patch-maintenance script for the pre-application baseline, not a post-application validation command.
