#pragma once

class MarioActor;

namespace smgpc::tests {
    // Runs against the live actor, restoring all touched fields and constants.
    void verify_original_mario_walk_parameters(MarioActor& actor);
}
