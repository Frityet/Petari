# Original NPC gaze and actor helper restoration

Recovered MR::calcPlayerFaceStareVector and calcPlayerFaceStarePos in canonical decomp/src/Game/Util/NPCUtil.cpp, then copied the exact methods into the native Game TU. The retail routines obtain the actual player Face0 joint, actor origin and supplied orientation; their behind-plane correction and return flag are preserved. This is the shared NPC gaze calculation used by Rosetta, with no character-specific position substitute.

The original compiler succeeds. Object comparison reports 93.64% for the vector routine. The position wrapper has the same extraction, vector calculation and translation addition; the compiler inlines TVec3::add instead of calling it (60.77% structural score), preserving its operations. No pursuit of instruction-perfect scheduling is needed for the functional port.

Also restored exact existing reference bodies for MR::extractMtxYDir, isIntervalStep(LiveActor), createPartsModelNpc, and invalidateShadowAll in the existing native provider TUs. Rosetta's existing original AstroDome and Epilogue companion TUs were copied into Game so its complete virtual behavior can link, without stage-specific constructor suppression.

The production smg-pc build succeeded with the recovered TurnJointCtrl and active original Rosetta/collector entries (gateway-wakeup-demo-20260912/original-app-sixth-build.json). Actual real-disc startup now passes the FIFO transition and stops while binding the original ErrorMessageWindow layout during GameSystem::init (original-app-second-run.json/log). No game frames, NPC behavior or original opening are claimed.
