# Native Wii Message Board capture omission

`src/Game/Scene/GameScene.cpp/.hpp` now excludes the OdhConverter include, draw-phase call, nonvirtual declaration and method definition when TARGET_PC is defined. The original non-PC path remains unchanged. This matches the explicit native NWC24System disabled policy; it does not claim an image was encoded or sent.

ODH/AJPG capture is requested by the original PowerStarList Wii Message Board export flow and passed to SendMailObj as its image attachment. PowerStarList.cpp is not yet imported natively, so GameScene was the sole active capture consumer. Ordinary game rendering and capture-screen effects are unaffected. If that UI is imported later, its optional Message Board export action must respect the same disabled platform policy.

The affected GameScene translation unit compiles. The two production paths above are the complete change; decomp reference is untouched. No new tests or shared build changes.
