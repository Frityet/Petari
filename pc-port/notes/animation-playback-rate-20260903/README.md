# Animation playback-rate diagnosis

## Measured cause of general slow motion

Before this change, the normal Gateway showcase advanced one game update for each rendered frame.
`AuroraRenderer::begin_frame` increments its frame index once and reports a
synthetic `1/60`-second delta, while both showcase loops invoke
`RuntimeContext::begin_frame` exactly once per presentation. The ordinary
application loop has the same relationship; its frame pacer limits fast frames
but skips missed deadlines rather than executing missed game ticks.

On the pre-change macOS Debug executable, a normal 241-frame Gateway session ran
at **18.8317 frames/s** with default VSync, measured over frames 61 through 241.
The otherwise equivalent VSync-disabled session ran at **17.2010 frames/s**.
These are whole-process observations, not proof of a particular GPU bottleneck;
they do rule out a simple VSync 30/60 setting as the explanation. A rate-one
animation advances only roughly 18 authored frames per wall-clock second in
these sessions, about 0.3 times the intended 60-tick progression.

The exact executable command, from `pc-port`, was:

```sh
SMGPC_AURORA_RENDER_STATS=1 \
  build/macosx/arm64/debug/smg-pc-showcase gateway \
  --disc '../Super Mario Wii - Galaxy Adventure (Korea).rvz' --max-frames 241
```

The second run additionally set `SMGPC_ENABLE_VSYNC=0`. A Python subprocess
reader timestamped each `[smgpc:render] frame=N` line with `time.monotonic()`.
The raw frame/time samples, command arguments, environment overrides, exit
codes, and steady-rate calculation are saved in `baseline-vsync.json` and
`baseline-no-vsync.json`, with complete output in matching `.log` files. The
runs did not use `--smoke`, which exits too early for steady timing. Both exited
successfully. Per-frame diagnostic output introduces some measurement overhead;
these figures describe the instrumented Debug runs rather than release speed.

## Current animation-clock chain

The source audit found no seconds-to-frames conversion within the active BCK
path that would explain this wall-time mismatch:

1. PC `LiveActor::movement` advances the actor animation once when its model is
   present and the stopped-animation flag is clear.
2. `advance_actor_animation` calls the original-shaped `J3DFrameCtrl::update`,
   whose first operation adds `mRate` to `mFrame`. This is the frame controller
   operation used by the original XanimePlayer, not a wall-clock delta.
3. `synchronize_actor_model_animation` copies that controller frame/rate/state
   to `LiveActorModel`. `LiveActorModel::bck_frame` returns the retained frame;
   it does not derive another frame from render time.
4. The renderer normalizes the frame with the animation's existing loop policy,
   then `OriginalJ3dJointTree` places that raw frame on its real temporary
   `J3DAnmTransformKey`. The original XanimeCore calculates the joints from it.

The original root `LiveActor::movement` updates ModelManager once per game
tick, and ModelManager advances the actual XanimePlayer. The native controller
owner remains structurally incomplete, as documented separately in
`../actual-mario-animation-owner-20260903/audit.md`, but the inspected clock
increments themselves are in animation frames, not seconds.

## CPU sample

A third normal session used `--max-frames 181`; after its 30th frame,
`/usr/bin/sample <pid> 3 1 -file <notes>/showcase-cpu-sample.txt` captured a
three-second call graph. `sample-command.log` and `profile-run.log` preserve
the capture status and session output.

Of 2,116 main-thread samples, 2,101 were beneath
`RuntimeContext::draw_3d_normal`. The dominant path was
`J3dModelRenderer::submit_mesh` -> CPU world-vertex conversion -> raster-color
lighting calculation. Eleven samples were in `RuntimeContext::begin_frame`.
Only one recorded path entered XanimeCore. This points to Debug CPU geometry
and lighting work as the major cost of this measured scene, rather than the
original animation sampler. Sampling is a statistical diagnostic, not a precise
per-function timing measurement.

This also supports decoupling the original 60-Hz game updates from expensive
render presentations: the current update work is much smaller than drawing.
Every due update must still execute the original per-tick logic in order;
increasing only animation rates would leave movement, cameras, nerves, and
event timing in slow motion. A clock design must retain missed ticks, preserve
fractional elapsed time, avoid charging scene-loading/paused time to gameplay,
and handle displays faster than 60 Hz without advancing the game too quickly.

## Separate original locomotion-rate gap

The PC `MarioAnimator::update` currently selects a single `Run` or `Wait` BCK
and leaves its rate at one. Original `Mario::decideWalkAnimation` instead sets
four track weights for the authored `基本` group (`WalkSoft`, `Walk`, `Run`,
`Wait`) and computes the group's playback speed from movement, stride lengths,
slope, acceleration, sinking, and other original state. Root source
`MarioWalk.cpp:346–410` ends by calling the actual lower player's
`changeSpeed(animationSpeed)`.

For the simple full-speed Mario case, `mWalkSpeed=1`, original movement constant
13, and the Run stride length 1.3 give the base expression
`0.5 * 60 * (0.01 * 13 / 1.3) = 3`. This is a source-derived example before
other modifiers, not a proposed fixed Run multiplier or a measured active
actor value. The stationary branch can use a rate of 0.33 with a normalized
multi-track frame domain. The proper correction requires the actual authored
player/group lifecycle and original decision routine; mapping these examples
onto the temporary single-BCK path would replace gameplay logic.

## Shared fixed-step clock foundation

`src/render/core/FixedStepClock.hpp` now supplies a pure elapsed-time accumulator:

```cpp
std::uint64_t advance(std::chrono::nanoseconds elapsed);
void reset() noexcept;
```

The returned count is the number of complete original 60-Hz updates due. The
caller runs each update with its original `1/60` delta; the clock changes no
animation rate. It retains every delayed update and the integer fractional
remainder. Splitting whole seconds before multiplying avoids overflow over the
entire nonnegative `nanoseconds` range and avoids rounding `1/60` to an integer
duration. A negative interval throws without changing accumulated time.

Elapsed time comes from
Aurora's game clock so pauses and its time scale remain authoritative. The
bootstrap tick is explicit in the caller; elapsed-time accounting by itself
produces exactly 60 ticks per second. Reset establishes a new scene epoch;
zero elapsed time during pause preserves the existing partial tick.

`tests/FixedStepClockTests.cpp` contains six regression groups. The first runs
the actual original `J3DFrameCtrl::update` at rate 1.25 under 18, 30, 60, 120,
and 144 presentation timestamps per second, requiring 60 updates and frame 75
for each schedule. The other groups exercise nanosecond boundaries and many
small intervals, retained stall ticks, pause versus reset, maximum and day-long
durations, and rejection without mutation. The target must link the original
`J3DFrameCtrl` provider.

The parent integrated this accumulator through `app/SimulationClock.hpp` into
the application and both showcase loops. Every due tick executes the complete
runtime movement, animation calculation, and camera phase before the batch is
drawn. The wrapper retains queued ticks when an exact capture, trace, launch,
or termination boundary must first be presented. The native six-group clock
test passed. No Game algorithm or playback multiplier was changed.

## Live verification after integration

The debug-only `SMGPC_DEBUG_SIMULATION_TIMING=1` diagnostic in Showcase reports
the logical tick, presentation count, actual BCK controller name/frame/rate/end,
and actor position after presentation. The existing renderer diagnostic still
reports its own presentation index, so it cannot by itself measure simulation
throughput. `sample_timing.py` timestamps both diagnostics and writes the raw
samples to local JSON and logs.

The updated normal 241-tick runs produced:

| Run | Logical sample range | Logical ticks/s | Presentations/s | Exit |
| --- | --- | ---: | ---: | ---: |
| Default VSync | 62–241 | 59.4522 | 17.9353 | 0 |
| VSync disabled | 62–241 | 59.7527 | 18.0260 | 0 |
| Real W input and screenshot | 60–241 | 59.7680 | 16.1803 | 0 |

The first two commands were:

```sh
python3 notes/animation-playback-rate-20260903/sample_timing.py fixed-step-vsync
python3 notes/animation-playback-rate-20260903/sample_timing.py fixed-step-no-vsync --no-vsync
```

These rates use the change in logged **logical tick** over the monotonic
timestamp difference, with presentation throughput calculated separately over
the same interval. Samples occur after drawing; variable final-frame latency
and measurement overhead account for the small difference from 60 in this
short interval. The game keeps advancing at its intended tick rate while Debug
rendering remains around 18 presentations per second.

All 67 default-VSync and 68 VSync-disabled samples retained `Wait.bck` at rate
1.0. Every consecutive sample advanced its frame by exactly the logical tick
difference modulo the original 180-frame loop, with zero mismatches. Both ended
at logical tick 241 and Wait frame 60.

The walking command was:

```sh
python3 notes/animation-playback-rate-20260903/sample_timing.py fixed-step-walk-active \
  --walk --screenshot notes/animation-playback-rate-20260903/walk-active-frame130.png
```

This macOS diagnostic script activates the process window and posts real W
keydown/up events to that process. It selected Wait -> Run -> Wait, retained
rate 1.0, and produced 21 Run presentation samples. Every pair within the same
animation advanced by the exact logical tick difference, including Run's
60-frame wrap. The requested tick 130 was presented exactly, with Run frame
55, and the final tick was exactly 241. The 1920x1440 screenshot was inspected:
Mario has a running pose and the authored planet geometry remains visible.
This checks animation presentation, not accurate original locomotion or jump
behavior. The initial attempt without activating the window produced only
Wait; it is retained as an input-delivery diagnostic and is not walking proof.

Keep this compact note, the sampling script, and regression sources in the
checkpoint. Full local timing logs, JSON samples, CPU profile, and screenshots
are supporting artifacts rather than required tracked files.

## Final runtime gates

After rebuilding `smg-pc-showcase` with the opt-in diagnostic, the following
programs ran serially with the real RMGK01 RVZ:

| Program | Result |
| --- | --- |
| `smg-pc-showcase title --smoke --max-frames 600` | Passed in 3 presentations, with original sky BCK/BTK packets |
| `smg-pc-showcase gateway --smoke --max-frames 600` | Passed in 4 presentations, with animated Mario, planet packets, GPU draws, gravity, and KCL probe contact |
| `smg-pc-gateway-spin-checkpoint-tests` | Passed real spin-unlock checkpoint |
| `smg-pc-mario-gateway-walk-tests` | Failed the existing real-planet KCL seam grounding assertion |

The showcase commands include `--disc '../Super Mario Wii - Galaxy Adventure
(Korea).rvz'`; the standalone tests use that path as `SMGPC_REAL_DISC`.
`runtime-gates.json` records complete arguments, exit codes, and elapsed time;
matching logs retain output. The walk failure remains a movement/collision
dependency gap and was not bypassed. The timing change does not activate
jumping or the original multi-track Mario animation owner.

The parent also built the full application and all selected test executables.
Eleven CPU regression programs passed: clock, original vertex buffer/Core,
Binder/KCL, Aurora native, and seven camera programs. `regressions.json` records
the executable hashes and exit codes for this checkpoint.
