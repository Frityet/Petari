# Complete original MapPartsUtil recovery

Restored all 58 retail functions in the existing original utility translation unit, then mirrored it to native unchanged. This closes real MapObjActor/SimpleMapObj prerequisites rather than adding extracted compatibility providers. Rawls's six indexed rail-rotation readers remain unchanged.

The placement readers fetch the authored integer column, leave the caller's output unchanged for a missing field or -1, and convert accepted integers to float for float outputs. The RotateTime wrapper deliberately uses the same RotateSpeed field as retail. Rail arguments go through their actual original rail owner methods. Rail guide creation calls the real SceneObj_MapPartsRailGuideHolder (0x56). Sensor type changes, rotation coordination, model/collision/rail clipping and box/sphere/joint shadows retain their original calls and ordering.

Original compiler succeeds. All 58 retail methods are covered: 57 score 96.67–100%; initMapPartsShadow scores 68.79% because this compiler inlines the same JMath vector subtraction and zero-vector constructor that retail calls out of line. Its shadow type selection, box max-minus-min, sphere radius multiplier 0.70710677, base-matrix argument and Move-joint offset are preserved. Weighted score is 96.8993%. This is functional/reference proof, not a rendered shadow or real map-parts gameplay claim. Native compilation also passes; no full builds or tests were run by this lane.

Publication paths:

- decomp/src/Game/Util/MapPartsUtil.cpp
- decomp/include/Game/Util/MapPartsUtil.hpp
- src/Game/Util/MapPartsUtil.cpp
- src/Game/Util/MapPartsUtil.hpp
- src/Game/MapObj/MapPartsRailGuideHolder.hpp (exact original declaration import; root owns subsequent full owner recovery)
- src/Game/MapObj/MapPartsRailPointPassChecker.hpp (exact original declaration import)

Header corrections remove four unused/non-retail JMapInfoIter overloads (rail guide, move speed, rail rotate speed/time), retain the actual actor/indexed overloads, and fix ShadowType's boolean return. No duplicate native providers were found for the newly recovered methods; the pre-existing original native TU already participates in Game compilation. No source-list/index/factory changes made.

The runtime still needs complete real rail mover/rotator/posture/guide owners before the unique planet creator can be enabled. Root is recovering StageEffectDataTable and the guide owner; Rawls owns RailRotator and seesaws. Those dependencies are explicitly retained, not replaced with empty success paths.

## Original RailPosture owner

Full MapPartsRailPosture source is also imported and compiles on both platforms. The apparent missing call parentheses in exeMove are an actual retail behavior: 0x8025E0F4 loads the member-function pointer and calls __ptmf_test, rather than calling isPostureTypeRailDirRailUseShadowGravity. An explicit `&MapPartsRailPosture::isPostureTypeRailDirRailUseShadowGravity` preserves that behavior under standard native C++. The original compiler still emits the member-pointer test. Do not replace this with a predicate invocation. Additional paths: decomp/src/Game/MapObj/MapPartsRailPosture.cpp and src/Game/MapObj/MapPartsRailPosture.cpp; the native header was part of the preceding declaration imports.
