#include "Game/NPC/MiiFacePartsHolder.hpp"

#include "runtime/RuntimeContext.hpp"

bool MiiFacePartsHolder::isInitEnd() const {
    if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
        return runtime->rfl().is_initialized();
    }

    return true;
}

bool MiiFacePartsHolder::isError() const {
    if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
        return runtime->rfl().has_error();
    }

    return false;
}
