# Camera and jumping demo bring-up, 2026-09-10

User priority is a new playable demo as soon as camera or jumping is ready; latest request asks for both. No playable frame has been verified yet.

The fresh debug showcase links with LLVM 23.1.0 and launches Metal against the real RMGK01 disc. Initial registration failures were diagnosed with LLDB, not suppressed: process-owned capture actors moved to original scene creation, and the demo director now registers only after its scene executor exists. Original CameraDirector/CameraCover are constructed from the real resources.

The next attempt hangs inside the existing decompiled MatrixControl constructor during MarioActor construction. loading-sample.txt proves its infinite loop. The process ignored SIGTERM while stuck in construction and was terminated with SIGKILL; demo-order-run.json records exit -9, not ordinary failure or success. MatrixControl is under reference-first correction.

The existing untracked package_walking_demo.py is preserved. Do not package or advertise this binary as working camera/jumping until frame and input verification pass.

## Original Mario startup follow-up

MatrixControl has been recovered in the decomp reference and copied into the port. The constructor now matches all Wii instructions and its lookup behavior passes a sanitized native probe. The rebuilt executable gets past that loop.

The next live run reached original MarioActor::init2 and crashed on a null GameSceneLayoutHolder in initLifeCount. Both generalized StageInitializationService and the bounded development scene now request the existing original holder through SceneObjHolder before player initialization. This invokes the actual unchanged HUD constructors. The first new boundary is FlyMeter's ordinary BRLYT 3D pane transforms, currently rejected by LayoutRuntime. The shared transform/parser/render path is being implemented; the user's latest instruction explicitly deprioritizes HUD work, so this work is limited to the required startup dependency.

The original 3D-transform exception was masked by a constructor-unwind lifetime error: C++ does not execute GatewayDemoScene::Impl's destructor when its constructor throws. Automatic reverse member cleanup deleted the original requirement holder before the scheduler disconnected remaining members. Initialization now catches failures inside the constructor body and invokes the same ordered scene teardown as ordinary destruction. DemoDirector also retires before its scene camera/player/execution owners. This preserves the first exception instead of suppressing a missing owner.

MarioEffect's packed numeric flag word also required the existing generalized Aurora PPC bitfield support to preserve retail byte significance on ARM64. Table values and Game method bodies remain unchanged. Isolated native and Wii evidence is in original-mario-effect-flags-20260910.

Current limits: camera/jump gameplay still unverified; no new app packaged or advertised as playable. Runtime logs in this directory retain actual crashes/failures rather than treating linked binaries as successful demos.

## First frame reached

The shared 3D-pane fixture passes, including original FlyMeter initialization and three Metal frames. The next live startup identified a native ResTIMG relocation defect: setResTIMG preserves the low32-bit displacement when replacing a record, but backward references were then zero-extended during 64-bit pointer addition, shifting the image by 4 GiB. Native signed32-bit displacement fields preserve the original 32-byte layout and address direction within the existing graphics heap's signed32-bit range contract. Serialized source validation still rejects out-of-range unsigned file offsets. JUTTexture's fallback expression also keeps the displacement signed.

The new texture regression executes original loadTexNo for both forward and backward image/palette aliases, including self-replacement. The full texture-resource fixture passes all six groups with real Mario.bdl TEX1 (22 textures, 101440 bytes). All five requested targets rebuild and link successfully in texture-fix-build.json. The next live showcase run completes original MarioActor initialization and authored placement, then stops at the first actual CameraDirector movement: CamKarikariEffector calls MR::isPlayerDead, whose stale bounded-port state provider is unresolved. player-state-stack.log records the original caller chain. No camera/jump frame or playable app is claimed.
