#pragma once

class MarioActor;

namespace smgpc::tests {
    // Exercises the original status stack on a fully initialized live actor.
    // Restores both stack-owner fields before returning, including on failure.
    void verify_original_mario_state_lifecycle(MarioActor& actor);
}
