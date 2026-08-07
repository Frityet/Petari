# PC Game versus root: route surface

The route-specific source differences are zero for the restored decomp files and the two support headers used by `PrologueDirector`:

| PC file | Root counterpart | SHA-256 | Result |
| --- | --- | --- | --- |
| `src/Game/Demo/PrologueDirector.cpp` | `../src/Game/Demo/PrologueDirector.cpp` | `12c049b3f73336f7eab8ef9a6f1c2c2c51333e8f0b8b1f1b31357e5d538f0da8` | byte-identical |
| `src/Game/Demo/PrologueDirector.hpp` | `../include/Game/Demo/PrologueDirector.hpp` | `6cb043e46e441f66a6efdb6e81ba918ba20e86445b71fc527daf9dcf875c4c6c` | byte-identical |
| `src/Game/System/StorySequenceExecutor.cpp` | `../src/Game/System/StorySequenceExecutor.cpp` | `39f09995744e5b56830f332944e40e328f66f370b5b97f7e5a562f1187f41387` | byte-identical |
| `src/Game/System/StorySequenceExecutor.hpp` | `../include/Game/System/StorySequenceExecutor.hpp` | `8cf1170fa7561db34fa5fdc308ee361efa6e9a14f02ea1790364b773688e8e47` | byte-identical |
| `src/Game/Util/SequenceUtil.cpp` | `../src/Game/Util/SequenceUtil.cpp` | `e25e94f7c0648c3a3ab8b61521899fbb0c412bb53943f88cba814d5a34a9741c` | byte-identical |
| `src/Game/Util/SequenceUtil.hpp` | `../include/Game/Util/SequenceUtil.hpp` | `5528893121755f41670af6729884edd8bc15d1b70fba34f828dbcf7c263043c6` | byte-identical |
| `src/Game/LiveActor/ActorCameraInfo.hpp` | `../include/Game/LiveActor/ActorCameraInfo.hpp` | `b584856af03dda3a4bc3e59f80d051fa72e172452ff3e50425867befae5e0920` | byte-identical |
| `src/Game/Util/JointUtil.hpp` | `../include/Game/Util/JointUtil.hpp` | `adb14eed7c625856da6213c07128e5bbed4f8a98daa594f56a103713e4495633` | byte-identical |

`src/Game/Util/SoundUtil.hpp` remains a broader PC compatibility declaration surface and is therefore not globally identical to the root header. This migration only changed two declarations there, adding the same default arguments used by the root header for `stopSystemSE` and `startAtmosphereSE`, so the exact root `PrologueDirector.cpp` compiles without edits.

The build explicitly excludes the exact root `StorySequenceExecutor.cpp` and `SequenceUtil.cpp` until their real retail dependency closure is available. The compiled implementation of the one required `SequenceUtil` request is in `src/compat/SequenceUtilCompat.cpp`; transition configuration and execution are in `src/scene/SceneTransitionRequestService.*`. No PC-only route implementation or debug route token remains under `src/Game`.
