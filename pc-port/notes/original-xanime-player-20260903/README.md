# Original XanimePlayer lifecycle

`src/Game/Animation/XanimePlayer.cpp` is copied byte-for-byte into the PC Game tree. Its existing header is also byte-identical to root. No algorithm, virtual behavior, rate, frame transition, or model-selection branch was changed. The two earlier `isRun`/`getSimpleGroup` extracts were removed from `compat/XanimeQueryCompat.cpp`; the remaining original string helpers still provide resource-table dependencies.

The whole original translation unit compiles against the current native Core, FrameCtrl, ResourceTable and real Model declarations. Its required external methods already have actual providers. `tellAnimationFrame` is defined in root and PC `MarioAnimator.cpp`, so it was neither reimplemented nor moved. No Mario or other actor was switched onto a new animation owner by this import.

## Source correspondence

Run from the repository root:

```sh
python3 pc-port/notes/original-xanime-player-20260903/verify-import.py
```

The verifier checks source/header byte equality, compiles both copies with the actual GC 3.0a3 compiler and configured Game flags, and compares instructions and relocations for 45 player/frame-controller functions: 1,253 unchanged PowerPC instructions and 134 identical relocations. `import-evidence.json` records the result and source digest. Original compiler commands, objects, and logs are under `build/original-xanime-player-20260903/`. This proves import correspondence; it is not a claim that this already-existing nonmatching root decompilation is a perfect retail instruction match.

The native source and regression file also pass isolated compile probes. The integrated xmake build passes for the original Player and Core test targets and the showcase. Both test executables pass: six Player lifecycle groups and nine Core groups. `native-gates.json` records the executable hashes. Game Player compilation explicitly retains the original no-contraction floating-point rule.

## Actual lifecycle coverage

The six regression groups use an actual `J3DModel` constructor, actual two-joint `J3DModelData`/`J3DJointTree`, original `XanimePlayer`/`XanimeCore`, actual `J3DAnmTransformKey` samplers, owned typed animation arrays, and a genuine `XanimeResourceTable` constructed with its original constructor.

1. Construction imports real bind transforms, creates independent clocks/tracks, and preserves the original shared-core joint-history alias.
2. Group and hash selection preserve identity; authored half-frame rates advance once per movement/calculation phase. `checkPass` temporarily restores the previous interval without changing the live frame. Re-requesting the current group retains its clock.
3. A two-track group blends actual clips of lengths 10 and 20 at matching relative phase. The expected translation combines independently known linear values, and later weight changes reach the actual model matrices. The original interpolation countdown and paused-loop rules are exercised.
4. One-shot termination preserves the player's original saved rate and samples the exact end. Hold policy, default-group restoration, named stopping, and countdown completion use the original methods.
5. Shared players with duplicated simple groups keep distinct actual animation bindings and can calculate through the real model. No name-to-file lookup is used.
6. Model replacement reconfigures real per-joint transform metadata after the caller enables that original capability; calculator attachment and clearing affect the replacement model's actual joints.

The tests explicitly own the arrays allocated by the original arena-oriented classes, whose destructors do not free them. Shared cores own only their newly allocated tracks. Alignment-placement matrix allocations use their matching delete overload. Each fixture restores J3D globals after use.

## Remaining production ownership boundary

The test resource table has a null holder and explicitly populated, already decoded groups. It exercises group/hash and direct typed-animation APIs only. It does not call `findResMotion`, `findStringMotion`, `changeSimpleBck`, name-to-file `changeTrackAnimation`, or other archive lookup paths. This is not a fake ResourceHolder or a replacement production resource provider.

The native archive service still declares a different global `ResourceHolder` layout from `Game/System/ResourceHolder.hpp`. Production use of the original resource-table constructor with archive lookup remains blocked until a real typed holder/resource lifecycle replaces that boundary. Likewise, the intentionally joint-only fixture does not claim material/shape decoding, a full loaded BMD owner, Mario's lower/upper player scheduling, or original actor animation activation. Those remain separate integration work.
