#include "Game/Screen/MiiSelect.hpp"

#include "Game/compat/RuntimeContext.hpp"

MiiSelect::MiiSelect(const char*) {
}

void MiiSelect::initWithoutIter() {
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->note_debug_event("MiiSelect initialized for FileSelector");
    }
}

void MiiSelect::collectValidMiiIndex() {
    ++mCollectValidMiiIndexCount;
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->note_debug_event("MiiSelect collected valid Mii indices for FileSelector");
    }
}

s32 MiiSelect::getCollectValidMiiIndexCount() const {
    return mCollectValidMiiIndexCount;
}
