# Original Mario stick input and forced grounding

Appended exactly two missing original methods to `decomp/src/Game/Player/Mario.cpp`, recovered from verified preflatten commit `e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4`. Existing reference bodies and headers remain untouched. No native production source changed.

Both full reference baseline/restored TUs freshly compile using current Wii headers/compiler. Against the verified RMGK01 DOL, `checkForceGrounding` matches 97.764046% (712/712 bytes), and `inputStick` matches 97.060610% (792/792 bytes). All 45 direct calls retain original order; referenced constant-value sets and external data relocations agree. Instruction review confirms original field offsets, mode masks and branches. Full native LLVM23 object compilation of the restored reference TU also succeeds against current native headers.

`inputStick` retains original axis scaling/clamping, magnitude dead zone, configured angle-margin and quadrant processing, native JMath atan/sin/cos table selection, angle history, 2D ground/air adjustment and 2.5D axis updates, then world-direction construction. The earlier native method threw for all 2D/2.5D modes instead of calling these original routines.

`checkForceGrounding` is void, as already declared: retail guard exits have no result contract and update discards no meaningful return. It preserves the original grounded/mode gates, floor/gravity projections, vertical correction and speed decomposition. Its ordered `!(fabs(value) < 30)` guard matches retail branch-if-not-LT, including unordered values. The earlier native copy instead used `fabs(value) >= 30`, which changes the unordered case.

Remaining fuzzy differences are branch layout, local-vector/register allocation, commuted scalar multiplication operands and one repeated half-pi load. This evidence does not establish bit-exact native floating-point behavior or live gameplay readiness. No existing retail-paired score regressed. Adding callers changes full-TU IPA in unchanged writeBackPhyisicalVector: MR::clamp becomes out of line, improving its retail score from 96.18802% to 97.61364%; baseline-to-restored is therefore not a byte-identical whole-object claim.

Parent owns native mirroring, owner closure and coordinated commit.
