# Read-only operator camera-basis audit

The operator's camera-basis algebra agrees with ordinary original Mario movement.
The observed interruptions in frames 11000–12500 have direct dialogue and pipe
state evidence; no production change or additional gameplay test was made here.

Run `python3 notes/original-controller-basis-audit-20260919/analyze.py` from the
repository root to reproduce `summary.json`, `samples.json`,
`state-transitions.json`, and `source-equivalence.json`. The input is the recorded
`notes/gateway-compat-20260919/content-world-first-catch-18000-actors.jsonl` from
main SHA256 `b38e90868eb8ab1f1cdc72487c293cf0f15a3e50c6fd8b14adc846bec7a7d123`.
The script hashes the trace and inspected source files in summary provenance.
It imports only the pure `controls` helper from the external operator, writes
only these evidence files, and never accesses the running controller input file.

## Camera and input comparison

`follow_actor.py::controls` reproduces the ordinary `Mario::calcMoveDir` branch
selection, camera-Z negation, side cross products, front sign and tangent-plane
basis. Its Gram-system solve correctly handles a nonorthogonal camera basis.
Across all 151 captured frames in the interval the basis determinant is at least
0.5814, and fresh algebraic controls reconstruct the projected target direction
to floating-point rounding. The target used for this calculation is the hole
waypoint `(13565.785,-10103.431,7954.287)` from the saved operator log.

The helper is not the inverse of the complete physical-input pipeline:
`Mario::inputStick` scales/clamps raw input, remaps angles around the authored
0.1-radian cardinal margin, and quantizes with original trigonometric tables.
`calcWorldPadDir` then applies the original X/Y margins 0.25/0.2 with 0.5 start
thresholds before calling `calcMoveDir(..., false)`. The offline comparison starts
with the recorded processed Mario stick, applies these latter margins, and uses
the captured camera vectors and smoothed movement-up vector. This explains why
raw physical controller coordinates should not be compared directly with world
pad direction as an exact linear mapping.

All 151 records are retained, including null direction comparisons for zero
vectors and all stopped/bound states. Of those,42 samples have speed>0.01 and
nonzero reconstructed and observed pad vectors. On this explicitly selected
moving subset:

| Comparison | Minimum dot | Median dot |
| --- | ---: | ---: |
| Margin-adjusted reconstructed direction versus actual world pad | 0.999591 | 0.9999965 |
| Actual velocity versus world pad | 0.621659 | 0.999199 |

Velocity is not required to align exactly during turning, falling, collision or
pipe exit. The low velocity-dot samples remain visible in `samples.json`; this
audit does not diagnose each one as a specific terrain response. The evidence
supports the absence of a systemic axis/sign inversion, not exact route following.

Stopped/bound samples are deliberately excluded only from the **moving subset**:
while pipe movement owns Mario, the recorded stick and world-pad values can stay
unchanged as the camera moves. Comparing that stale direction with a newly
reconstructed basis produces lower dots (down to 0.786793 over all records).
Those are preserved in the unfiltered statistics, not removed as bad data.
The tiny-vector fallback threshold differs between the Python helper and the
Game helper, but neither basis vector approaches degeneracy in this interval.

`source-equivalence.json` reports that `calcWorldPadDir` is identical to the donor
with whitespace removed. `calcMoveDir` is not text-identical: native source uses
early returns, different temporary names/vector cross helpers, and the explicit
PPC `_10_LOW_WORD & 0x1000` mask for canonical `_10._13`. The inspected ordinary
branch algebra is equivalent. This is a read-only semantic comparison, not a new
whole-function retail matching claim.

## Recorded progression, not inferred terrain failure

- At frame 11000, Tico 892 is in Talk, Mario's physical controller is
  `(-0.0329,-0.999459)`, but processed stick, world pad and velocity are zero.
  Mario remains at `(14045.205,-11773.203,5136.242)` through frame 11800.
  `MarioActor::getStickValue` explicitly suppresses input under original input
  disable/draw-state gates. The trace establishes the coincident Talk state,
  not which private gate bit was active.
- At frame 11860, Tico 892 transitions to Wait and ordinary movement resumes.
- Pipe 697 transitions Wait→Ready at 12020, PlayerIn at 12050, and PlayerOut at 12120.
- Between sampled frames 12110 and 12120, Mario moves exactly from pipe 697's
  position `(12761.115,-11282.014,6032.298)` to pipe 699's position
  `(16877.883,-10250.962,6408.290)`, a 4260.541-unit change.
- Both pipes enter Invalid at 12200. Rabbit 902 transitions Hide→Appear at 12200 and
  Runaway at 12250. These are observed original states, not operator-set states.

Original `EarthenPipe::exePlayerIn` copies the paired pipe's base matrix into the
bound player transform. `exePlayerOut` then issues the authored exit force with
input temporarily off and writes its B switch. The camera and cached-pad changes
around 12120 therefore coincide with a real pipe transit. A straight tangent
controller aimed at the hole can cross a pipe; this operator has no obstacle or
portal avoidance and does not prove that the hole route is navigable directly.

Root separately recorded the earlier competing-driver error and fixed the
external producer lock, then advanced post-catch Tico dialogue. Those operator
corrections are not changes to Game, the camera, or the input compatibility layer.
This audit did not create a producer, perform a build, use the GPU, or modify
production source.
