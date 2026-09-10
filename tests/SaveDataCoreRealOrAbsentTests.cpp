#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "runtime/RuntimeContext.hpp"

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

template <typename Operation>
void require_process_owner(Operation&& operation, std::string_view message) {
    auto unavailable = false;
    try {
        operation();
    } catch (const std::logic_error& error) {
        unavailable = std::string_view(error.what()).find("original process save sequence") != std::string_view::npos;
    }
    require(unavailable, message);
}
}  // namespace

int main() {
    require(smgpc::runtime::RuntimeContext::try_instance() == nullptr,
            "the absence fixture must not have a runtime or mounted save resources");
    require(SingletonHolder<GameSystem>::get() == nullptr,
            "the absence fixture must not have an original process GameSystem");

    require_process_owner([] { static_cast<void>(GameDataFunction::getCurrentGameDataHolder()); },
                          "current data requires the process sequence's actual current UserFile");
    require_process_owner([] { static_cast<void>(GameDataFunction::getSceneStartGameDataHolder()); },
                          "scene-start data requires the process sequence's actual backup UserFile");
    require_process_owner([] { static_cast<void>(GameDataFunction::getUserName()); },
                          "user-name access must not fabricate a current file without the process sequence");
    require_process_owner([] { static_cast<void>(GameDataFunction::getSysConfigFileTimeAnnounced()); },
                          "system configuration requires the process sequence's actual SysConfigFile");

    require(SingletonHolder<GameSystem>::get() == nullptr && smgpc::runtime::RuntimeContext::try_instance() == nullptr,
            "failed save-data queries must not manufacture a process or runtime owner");

    // SaveDataHandleSequence now executes its complete original constructor and
    // state machine. Resource loading, NAND completion and save UI transitions
    // require the real process startup graph and are not exercised here.
    std::cout << "Save-data process-owner absence checks passed\n";
    return 0;
}
