# Original shadow-line ownership

## Problem and boundary

The native shadow CSV parser already retained line widths and original preceding/self endpoint indices, but `ShadowControllerOwnership` did not create a `ShadowVolumeLine` drawer. The four original utility entry points needed for programmatic graphs (`initShadowController`, `addShadowVolumeSphere`, `addShadowVolumeLine`, `setShadowDropDirectionPtr`) were absent. Programmatic volumes also incorrectly inherited the CSV-specific 100-unit draw offsets instead of the original drawer constructor's zero offsets.

The implementation supplies a complete-definition add boundary, actual original drawer ownership, same-list indices including self, and borrowed cross-actor controller identities. It does not copy endpoint/controller state. Endpoint and direction owners must outlive their borrowers, matching the original pointer contract. Original name resolution occurs as if the new controller has already been added, preserving the sole-controller shortcut, prior-name precedence, self references and unresolved/null results. CSV construction keeps its explicit 100-unit defaults and original row-order lookup; a forward name does not become valid merely because its target is parsed later.

`ShadowVolumeLine.cpp/.hpp` are imported from canonical `decomp` commit cc77fe564c4da3f8ee35605134e2add83628022a. The native source adds only the CP932 helper include and wraps its single Japanese constructor label. The recovered drawShape has 97.91% MWCC text similarity; recovery and original DOL checks are recorded in `notes/original-punching-kinoko-20260919/`. No actor behavior, factory selection, or gameplay state is changed by this work.

## Validation scope

A new debug-only target uses the previously committed synchronous OriginalProcess observer. It runs the real stage process, actual scene Game heap, original shadow holder/list, loaded sphere model, original line drawer and actual GX display-list writer. Its temporary actors are retired before another gameplay frame. Checks cover programmatic and CSV defaults, cross-actor/prior/self/unresolved bindings, mutable borrowed direction, analytic original shape commands, degenerate no-shape returns, rejected-definition publication, and actor/scene retirement. It preserves and restores the actual GX display-list shadow state and does not submit diagnostic geometry to the GPU.

This is bounded subsystem integration evidence, not PunchingKinoko placement support or Gateway chase/Rosalina progression. The older standalone shadow fixture remains unchanged; its missing original scene language/resource owners are not bypassed.

## Results

The root coordinated a normal shared Xmake build of `smg-pc`, `smg-pc-original-process-shadow-line-tests`, and `smg-pc-original-layout-group-tests`; it passed in 9.372 seconds. No concurrent production edits occurred during this build.

`process-probe.json` and `process-probe.log` record **PASS**, exit 0, 120 completed original frames in 8.114 seconds. At frame 54, ten new actual controllers and all owned drawers passed the graph/default/shape checks. The original drawer emitted two four-vertex quads and its ten-vertex strip with the analytically expected big-endian float positions, then changed extrusion planes when the shared borrowed direction changed. Parallel direction and coincident endpoints emitted no geometry. Original pending-holder entries disappeared during actor retirement; normal process shutdown retired the original holder and all shadow owner state. The process exited normally (PID 56851), with no timeout termination.

Test executable SHA256: `84d05e611aff06a53846831d78c80b5df143ef24e5bf6d1db29241518eda42e1`.

`donor-equivalence.log` records the exact native/canonical comparison. The current decomp HEAD was f2f8112331e126ff597f9760fd8785719e8a47ab (the later unrelated matrix correction); the ShadowVolumeLine donor remains the published cc77fe5 recovery. Header byte equality and normalized CPP equality both pass.

The root publishes this checkpoint; no staging or commits were performed by this agent.
