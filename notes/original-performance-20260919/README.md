# Original process performance

The user reported that the game runs very slowly. This was not intentional
slow motion. The currently configured debug build had optimize_debug=false.
The next build enables ordinary compiler optimization while retaining debug
guards, assertions and existing per-file floating-point contraction rules.
No simulation-rate multiplier, omitted gameplay update, reduced scene content,
or frame-skipping workaround is added. The build option defaults to true;
explicit `xmake f --optimize_debug=n` remains available for unoptimized debugging.

## Before measurement

`../gateway-compat-20260919/perf-neutral-debug-600.json` records a clean
600-completed-frame run in36.8418seconds (16.286fps averaged over the whole
process, including startup and teardown). Exit0, no timeout, PIDgone. All
controller scripts, actor/layout traces and live input were disabled; screenshot
frame was set beyond the run. This establishes that those diagnostics were not
necessary to reproduce the slowdown. The8second macOS sample ran inside this
process; sampling overhead is included, so this is not a precise unsampled
steady-state baseline.

`debug-sample.txt` identifies66% of main-thread samples in the original render
end waiting for Aurora FIFO completion. On the FIFO thread,38% of samples are
in require_draw_array_spans, whose per-vertex bounds-validation loop is compiled
without optimization. The render worker spends most of its samples waiting for
work. These are wall sampling proportions, not additive CPU utilization.

## Optimized build

The complete library closure and main/focused targets compile and link with
-O2 -g and SMGPC_DEBUG_BUILD, with no NDEBUG or fast-math. Effective original
Game and Aurora commands are recorded in o2-effective-flags.json; Binder and
GameMathCompat retain their per-file -ffp-contract=off. A briefly duplicated
Xmake request was canceled with exit143 and an empty log; the owning agent
completed and rechecked dependencies before all final links. Sources stayed
frozen. Main SHA256:
dcc7420463f7095aff46eb5f709cc2cbbf7814a3d70192911c1860b2a7d7279e.

The same600frame neutral run now completes in13.0929seconds (45.826fps whole
process including startup/teardown), exit0/PIDgone. The optional debug timing
summary measures12.3153seconds inside the frame loop and53.807fps over its
final300frames, with no sampling profiler attached. These are different
measurement scopes; neither is labeled a steady60fps result.

The later3200frame gameplay replay also exits normally with all3200frames
completed in83.5551seconds. Its initial actor-test link and an8second sampling
profile overlap only the early part of the run. The final300frame window, after
those activities ended, averages34.025fps. That window includes the requested
frame3000 PNG capture and therefore includes capture/readback/encoding cost.
The screenshot was viewed: Mario and the authored planet/rocks/UI render after
the four scripted movement directions. This is not proof of60fps gameplay; the
remaining Aurora vertex-array span validation cost is documented as follow-up.
The user accepted34fps and requested returning to gameplay content; no decoder
optimization or simulation change was made.

`benchmark-summary.json` keeps the measurements in their original scopes.
Profiles/build logs are published compressed with deterministic gzip metadata.
