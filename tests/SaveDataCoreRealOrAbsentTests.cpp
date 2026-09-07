#include "Game/System/SaveDataHandleSequence.hpp"
#include "compat/SaveDataHandleSequenceCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

void require_unavailable(const std::function<void()>& operation, std::string_view message) {
    auto unavailable = false;
    try {
        operation();
    } catch (const std::logic_error&) {
        unavailable = true;
    }
    require(unavailable, message);
}
}  // namespace

int main() {
    require(smgpc::runtime::RuntimeContext::try_instance() == nullptr,
            "the regression fixture must not have a runtime or mounted save resources");

    auto local = SaveDataHandleSequence{};
    require(local.mSysConfigFile == nullptr && local.mCurrentUserFile == nullptr && local.mBackupUserFile == nullptr &&
                local.mSaveDataHandler == nullptr && local.mNANDErrorSequence == nullptr && local.mTempBuffer == nullptr,
            "constructing the unavailable sequence must not allocate a synthetic save core");
    require(!smgpc::compat::try_initialize_save_data_ui(local),
            "the capability probe must report that retail save UI backing is absent");
    require(local.mSysConfigFile == nullptr && local.mCurrentUserFile == nullptr && local.mSaveDataHandler == nullptr,
            "the capability probe must not mutate the absent sequence into a partial implementation");

    require_unavailable([&] { smgpc::compat::ensure_save_data_core_initialized(local); },
                        "partial save-core initialization must be explicitly unavailable");
    require_unavailable([] { static_cast<void>(smgpc::game::save_data_handle_sequence()); },
                        "the global sequence accessor must be unavailable without retail backing");
    require_unavailable([&] { local.update(); }, "sequence update must not be a no-op fallback");
    require_unavailable([&] { local.draw(); }, "sequence draw must not be a no-op fallback");
    require_unavailable([&] { static_cast<void>(local.isActive()); },
                        "active-state queries must not return a fabricated false");
    require_unavailable([&] { static_cast<void>(local.isPermitToReset()); },
                        "reset permission must not return a fabricated true");
    require_unavailable([&] { static_cast<void>(local.getCurrentUserFile()); },
                        "current-file access must not return a fabricated null");
    require_unavailable([&] { static_cast<void>(local.getHolder()); },
                        "holder access must not return a fabricated null");
    require_unavailable([&] { local.exeNoOperation(); },
                        "the no-operation nerve must not silently stand in for the retail state machine");
    require_unavailable([&] { local.startPreLoad(); }, "preload must be explicitly unavailable");
    require_unavailable([&] { local.startSave(false, false); }, "save must be explicitly unavailable");

    std::cout << "Save-data core real-or-absent tests passed: 14/14\n";
    return 0;
}
