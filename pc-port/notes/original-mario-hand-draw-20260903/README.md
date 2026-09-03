# Original hand pose and display-list setup

Recovered `MarioActor::updateHand` and `createRainbowDL` in root, then copied the complete files to the native tree. The original hand update reads the authored PartsControl translation, applies the original rounding/clamp, and changes visible hand joints. It retains original hidden/model/movement-state decisions. The display-list initializer constructs the original 64 TEV-color lists with the original color bits and eight alpha levels.

`DLchanger.hpp` now describes the actual pointer-bearing buffer array, count/current-buffer fields, inline constructor and copy routine. Root `MarioActorDraw.cpp` uses this actual class in its existing initializer and `addDL`, replacing fabricated derived byte-layout structures. The constructor's original allocation null check is restored. No animation rate or hand pose is invented.

`verify-original.py` uses the configured GC3.0a3 compiler and original RMGK01 split objects (DOL SHA1 25c5959534b3c21246c6c7e42021b916b41fb578). Final comparisons:

- updateHand: 99.8%, 540/540 bytes; original call order and constant values.
- createRainbowDL: 99.349594%, 492/492 bytes; original call order, color construction and allocation/write flow. Differences are register allocation in the final buffer copy.
- DLchanger::addDL: 100%, 36/36 bytes, typed field access.

The existing whole initDrawAndModel is 97.65101% after using the actual DLchanger constructor. That inherited body still differs in functor constness and register allocation; this is not an exact retail initializer claim. The earlier architecture-only proof in original-actor-model-retirement describes its pre-DLchanger header snapshot and remains available at its publication commit.

Native actor/model activation is still being linked; these compiler results do not establish jumping or a live gameplay result.
