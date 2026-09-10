# Original camera view publication

The first rendered frame showed only sky. LLDB at original Mario silhouette draw showed `j3dSys.mViewMtx` still identity while the actual CameraContext held a finite translated view. `MR::loadViewMtx` routed to a disconnected native `sViewMatrix` that no original model read.

The compatibility helper now copies into and reads the actual `j3dSys.mViewMtx`, matching the original `decomp/src/Game/Util/CameraUtil.cpp:152` copy. This applies to every original model and camera category. No Game source changed. Evidence: mario-draw-matrices.log and native-layout-first-present.png.

The user requested an immediate preview to report remaining issues; this handoff does not claim completed movement/camera fidelity or Gateway gameplay.
