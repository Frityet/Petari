#pragma once

class MarioActor;

namespace smgpc::tests {
    // Preserves the original camera cases while borrowing the real, already
    // active scene demo. Restores all player fields before the next frame.
    void verify_original_process_mario_camera(MarioActor& actor);
}
