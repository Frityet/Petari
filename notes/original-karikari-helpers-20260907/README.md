# Final retained Karikari helpers

The parent's fresh showcase link retained only two missing helpers from the restored Karikari state graph. Both were copied verbatim from their existing original decomp sources into the corresponding existing native providers; no Game source or other unused Karikari dependencies were changed in this step.

- `MR::isNearZero(const TVec2f&, f32)` retains four ordered, inclusive component-bound comparisons. It preserves the original axis-aligned tolerance region, zero/negative tolerance behavior and unordered floating-point comparisons.
- `MR::startDPDHitSound()` retains the existing ME selection: ME_DPD_HIT when hasME is true, otherwise SE_SY_DPD_HIT, followed by the original secondary-controller sound call CS_DPD_HIT. It forwards to the current native audio and actual connection-state providers; it adds no replacement sound result or actor-specific behavior.

Fresh full original Wii compilation succeeded for MathUtil and SoundUtil. The two helper bodies compare at 100% (84 bytes) and 99.333336% (120 bytes), respectively; see wii-proof.json, full objdiffs and compiler manifests. Source-manifest.json records exact copied-body hashes.

Exact native syntax checks passed for both provider files and the extended GameMathRotationTests.cpp. The near-zero regression covers inclusive signed corners, the next representable value outside every signed axis bound, signed zero, negative tolerance, NaN components and NaN tolerance. That unchanged test function was extracted into a focused notes-only executable linked against the actual compiled production GameMathCompat.cpp with unused sections removed. Compilation, linking and execution all passed; see native-boundary-proof.json and near-zero-boundaries.log. The normal repository math target contains the same regression for the parent's coordinated test run.

No root Xmake build or source-selection edit was performed by this subtask, and no additional Karikari/Fur subsystem work was started.
