# Gateway compatibility expansion, 2026-09-19

Goal: execute the original Gateway opening (Mario wakes), bunny chase and Rosalina appearance through the original GameSystem/GameScene and authored demo actors. General compatibility fixes only; no stage-specific timing, fabricated progress or substitute actors.

## Starting state and reproduction

Parent HEAD was `4bd2fe77e`, Aurora `721b9e6`; the existing build-tool migration and unrelated dirty files are recorded in `starting-status.txt` and preserved. The existing production executable predates the migration. A fresh baseline run of that executable on the Korean RVZ reached 300 original process frames. `baseline-frame240.png` reproduces the missing planet view and overlapping/upside-down Mario models. Process exit then aborted; frame-loop completion is not clean process completion or gameplay success.

The first build failed before compiling because the selected toolchain did not provide the required sibling `llvm-config`. The initial configuration lacked an explicit LLVM SDK; the exact prior cached compiler path was not preserved. Reconfiguring with the installed `/opt/homebrew/opt/llvm` SDK succeeds (see `toolchain-config.log`); no toolchain source changes were required. Build logs record each subsequent result independently.

`run_original.py LABEL [FRAMES]` captures a fresh native console directory, binary SHA256, command, screenshot, runtime log, exit status and timeout status. It supplies only a stage/scenario selection through the existing frontend. It does not set story flags or inject gameplay state. Disc assets and native save directories are not committed.

## Active investigations

- Missing original `DemoGroup` and `DemoSubGroup` factory entries prevent Ticos from registering for their authored guide demo. See `opening-factory.md` and LLDB evidence.
- Hidden actor draw membership must follow the original connect/disconnect requests; merely changing the hidden-model flag leaves original DrawBuffer lists drawing the actor.
- Explicitly disabled audio output still needs the original logical scene state used by unmodified SoundUtil.
- A bounded clean exit currently aborts with an invalid host free after GameSystem stops; debugger evidence retained for ownership diagnosis.

Build success and individual fixes are not evidence of the full requested wakeup-to-Rosalina sequence. Validation results are appended as obtained.

## First validated compatibility checkpoint

The restored original demo groups now register the Ticos and activate the authored
27-part `TicoGuideDemo`. Original `DemoPlayerKeeper` requests `MarioDemoPos` and
the `demomeettico` animation. That exposed a wrong native gravity restriction:
the original `findNamePosOnGround` deliberately queries gravity with a null
requesting NameObj and an explicit position. The compatibility query now passes
host zero through to the real scene gravity manager, matching `GravityUtil.cpp`.
The position-less LiveActor overload still requires an actor, and absent scene
gravity owners still fail explicitly.

`HeavensDoorDemoObj` and `EarthenPipe` source/header pairs are exact copies from
the existing decompilation. Their original factory rows and the original pipe
mediator scene-object constructor are registered; no new decompilation was needed
for these classes. This enables their authored placement, not verified tower
progression or pipe traversal. The submodule source head `024901ced` is already
published on `pcp-decomp`; the pre-existing dirty parent gitlink is preserved.

The native area-creator table is process metadata. It now uses host allocation
even when first queried under a Game heap, preventing its static destructor from
freeing retired Game storage as host memory.

The fifth and sixth production builds pass. `second-integrated.json` records a
360-frame original process run with clean exit 0 and the actual wakeup closeup
on the grass; `pipe-import.json` repeats the same clean bounded run with the pipe
imports active. Mario remains incorrectly blue in these screenshots. The longer
`opening-long.json` run reaches the first authored dialogue and fails when its
ordinary volume-preset request asks for the still-absent AudSystem.

Focused tests pass: `visibility-test3.log` checks actual original category
membership/callbacks, repeated hide/show, dead/clipped actors, hidden animation,
deferred cancellation and 16 scene lifetimes. `gravity-test.log` checks real
empty/populated gravity managers, null-requester position queries, host filtering,
priority/type selection and explicit missing-owner failure (`--queries-only`).
The old standalone test fixtures required updating to construct the original
GameSystem/controller/Scene owners; production ownership checks were preserved.

## Authored lighting, plants and native input checkpoint

The real original scene now owns its authored root/child stage-light cache. The
previous all-blue Mario was traced to uninitialized actor light data, not a
texture or shader channel swap. Original GX diffuse, ambient, point and coin
specular calls and LightDirector player registration are restored in native
providers; the three corresponding Game sources are exact reference copies.
See ../original-player-material-state-20260919/ for live register, command-byte,
scene-lifetime and image evidence, including the broader standalone fixture's
separate missing-language-owner failure.

PlantGroup and PlantMember were recovered in decomp first (98.22674% text and
100% numeric constants) and imported exactly with CutBushModelObj. Their three
original factory variants and three existing SimpleMapObj prop rows are now
registered. The missing general makeAxisCrossPlane body follows the original
math and passes winding, polar-normal and inclined-plane tests. This adds real
plant/scenery placement; rabbit reveal still follows its authored area/pipe
switches and is not inferred from visible plants.

`input-light-plant-ui.json` records a 4500-frame original opening with exit0 and
normal Mario colors. The later live debugger process PID6264 is a separate
diagnostic session, possibly relaunched by Computer Use; it is not covered by
that runner's exit status. It received actual SDL Return events and published
A to original KPAD/WPad. TalkStateEvent requires a held A sample after the
trigger frame, which the short automated tap did not establish. That process
later exposed a FIFO breakpoint exactly one byte before the end of a vertex
command, despite the following byte being present in the retained stream.
See ../original-fifo-command-boundary-20260919/ for its boundary capture.

The runner now records start time and PID before waiting, as well as final
status, so later live app launches can be distinguished from bounded runs.
