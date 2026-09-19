# External controller mapping audit

The copied operator's camera/gravity basis agrees with the ordinary 3D
`Mario::calcMoveDir` construction. The concrete operator error was treating
desired **processed** stick axes as raw WPAD axes. Corrected only
`../follow_actor.py`; the original/staged chase scripts and all production Game,
compatibility and Aurora code are unchanged by this task.

The native frontend sends the script values directly to
`WpadService::set_sub_stick`. WPadStick copies them without a hidden axis flip.
Before `calcMoveDir`, the original game applies:

1. `Mario::inputStick` (`src/Game/Player/Mario.cpp:1533`): gain 1.5, independent
   axis clamp, magnitude clamp, and the authored angular margin.
2. `MarioModule::calcWorldPadDir` (`src/Game/Player/MarioModule.cpp:258`): axis
   margins before the final camera-relative basis.

Both authored MarioConst tables use angular margin 0.1 rad, X margin 0.25, Y
margin 0.2 and X/Y activation thresholds 0.5. The corrected external operator
inverts those ordinary-input transfers before publishing its controller file.
It keeps pre-clamp magnitude circular and picks the nearer reachable angle at
the original discontinuities. It leaves a two-bin guard around those branch
boundaries for table/serialization rounding. This does not bypass input gates,
retained movement direction, alternate movement modes, terrain or collision.

## Actual trace comparison

Run `python3 notes/demo-system-verification-20260919/physics/analyze_controller.py`.
The script reads the actual game log's accepted input-file revisions, rather
than assuming that an operator write was consumed on its intended first frame.
Input log and trace hashes are in `controller-mapping-baseline.json`.

Frames 19000–29500 contain 1,043 retained samples and eight exclusions for zero
raw or processed stick. No inactive-span or zero-pad sample enters the vector
statistics. This is the third rabbit (id 912) chase in the unchanged baseline.

| Comparison | Median angle error | Maximum angle error |
| --- | ---: | ---: |
| Original input shaping of accepted raw axes → captured processed stick | 0.0104° | 0.0486° |
| Camera basis using raw axes directly → captured world pad | 5.7873° | 28.4304° |
| Original shaping plus camera basis → captured world pad | 0.1004° | 24.3677° |
| Camera basis using captured processed stick plus module margins → world pad | 0.0954° | 24.3862° |

The continuous analysis intentionally uses double trig; its small processed
stick residual includes the original JMath table/float rounding. The larger
world-pad outliers remain recorded. The trace captures state after the frame,
while the original direction calculation can update smoothed gravity during
the frame. This audit does not turn those outliers into a claimed engine defect
or a claim of exact per-instruction reconstruction.

## Why this does not prove the third chase is solved

Across those active samples, median target distance is 2,059.96 units, but its
projection onto Mario's input tangent plane is only 66.77 units. The median
dot product between normalized target displacement and `movement_up` is
−0.999474: the rabbit is almost directly inward relative to Mario's local up.

At frame 26000, distance is 2,052.29, tangent separation 76.93, and radial cosine
−0.999297. The target is at `(14764.23, -10678.03, 6775.18)` while Mario is at
`(16654.98, -10105.58, 7331.26)`. These are actual actor positions, not raw local
placement rows. The median velocity-versus-pad dot is 0.0318; commanding a
heading does not establish the resulting velocity or a traversable path.

This direct-target operator has no terrain planner. It updates at ten-frame
trace intervals, while Mario can move roughly 130 units in that interval—more
than the late chase's small tangent offset. An almost radial target makes the
chosen tangent direction sensitive to that delay. The data are consistent
with an unsuitable direct surface route or a collision/state problem, but
do not identify the physical cause. No teleport, forced catch, state write or
new gameplay behavior was introduced to compensate. The newly corrected
collision executable must be evaluated separately.

## Offline regression

- `controller-test-baseline.log`: the original `controls()` fails the normal
  camera/20° target case with 3.5128° of processed heading error.
- `controller-test.log`: all three new tests pass, including 1,440 quadrant and
  magnitude samples and exact six-decimal script serialization.
- `controller-route-test.log`: all 14 existing sequential route tests pass.
- `controller-test-initial.log` preserves initial test-fixture mistakes: the
  camera fixture assumed the wrong world sign, and the expected maximum error
  ignored the larger genuine angular gap at partial magnitude 0.75. Those test
  expectations were corrected against the original scalar transfer; no game
  or operator guard was weakened.

Run the green regressions with:

```sh
python3 notes/demo-system-verification-20260919/test_controller_mapping.py
python3 notes/demo-system-verification-20260919/test_sequential_route.py
```

The operator fix has **not** been exercised in a new live gameplay run. It is
an evidenced correction to external control math, not completed demo proof.
