# Original shared angle and vector helpers

The native normalizeAngleAbs wrapper used fmod and returned zero for TWO_PI. The original helper has an inclusive upper endpoint and retains TWO_PI, including when reducing a positive multiple of a full turn. That difference feeds circular interpolation and angle predicates used throughout camera and movement code.

GameMathCompat now contains the complete unchanged root normalizeAngleAbs, isAngleBetween, blendAngle, convergeRadian and two-dimensional normalize bodies. The additional helpers close actual camera references; there are no camera-type branches, rate multipliers or alternative implementations. `source-correspondence.json` records SHA256 hashes of the five complete bodies copied from root MathUtil.cpp. Root Game code was not changed.

The existing GameMathRotationTests fixture passes after adding inclusive full-turn/signed-zero cases, interpolation across the wrap and within an ordinary interval, convergence with overshoot and wraparound, and real 3-4 vector normalization. Existing quaternion, rotation, matrix, vector and trigonometric table checks remain active. Build/run from pc-port using `xmake build smg-pc-game-math-rotation-tests` then `build/macosx/arm64/debug/smg-pc-game-math-rotation-tests`.

The test uses the same dead-code removal as the production showcase and other bounded native fixtures. Without it the static Game archive pulls unrelated, still incomplete Mario providers into this math-only executable. This test does not claim the full showcase links or that jumping is active.
