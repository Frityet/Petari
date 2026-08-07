# Retail J3D frame-controller compatibility

The previous host declaration was a passive bag of fields with non-retail
default state. It now exposes the retail-shaped `J3DFrameCtrl` ABI and actual
controller behavior:

- retail loop-mode initialization;
- one-shot/reset/loop/reverse update rules;
- state bits for stop and loop passage;
- forward, reverse, and wrapped `checkPass` intervals;
- the normal frame/rate/start/end/loop accessors used by exact Game sources.

The implementation is generalized JSystem compatibility in
`src/compat/J3DFrameCtrlCompat.cpp`; no screen, actor, animation name, or route
is special-cased.

## Verification

The focused test covers initialization, one-shot end clamping, and both sides
of a loop wrap:

```text
$ g++ -std=c++23 -DTARGET_PC -Isrc -Iaurora/include \
    tests/J3DFrameCtrlTests.cpp src/compat/J3DFrameCtrlCompat.cpp \
    -o /tmp/j3d-frame-ctrl-test && /tmp/j3d-frame-ctrl-test
J3DFrameCtrl tests passed: 3/3
```

The normal target also passed after the exact ActorSensor boundary settled:

```text
$ xmake build smg-pc-j3d-frame-ctrl-tests
build ok, spent 3.08s
$ xmake run smg-pc-j3d-frame-ctrl-tests
J3DFrameCtrl tests passed: 3/3
```
