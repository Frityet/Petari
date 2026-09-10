# Camera and jumping demo bring-up, 2026-09-10

User priority is a new playable demo as soon as camera or jumping is ready; latest request asks for both. No playable frame has been verified yet.

The fresh debug showcase links with LLVM 23.1.0 and launches Metal against the real RMGK01 disc. Initial registration failures were diagnosed with LLDB, not suppressed: process-owned capture actors moved to original scene creation, and the demo director now registers only after its scene executor exists. Original CameraDirector/CameraCover are constructed from the real resources.

The next attempt hangs inside the existing decompiled MatrixControl constructor during MarioActor construction. loading-sample.txt proves its infinite loop. The process ignored SIGTERM while stuck in construction and was terminated with SIGKILL; demo-order-run.json records exit -9, not ordinary failure or success. MatrixControl is under reference-first correction.

The existing untracked package_walking_demo.py is preserved. Do not package or advertise this binary as working camera/jumping until frame and input verification pass.
