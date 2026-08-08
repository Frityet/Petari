#include "Game/NameObj/NameObjExecuteHolder.hpp"

#include "runtime/RuntimeContext.hpp"

#include <stdexcept>

namespace {
    smgpc::runtime::SceneScheduler &require_scheduler(NameObj *object) {
        if (object == nullptr) {
            throw std::invalid_argument("Temporary draw connection requires a NameObj.");
        }
        if (auto *scheduler = smgpc::runtime::try_active_scene_scheduler(); scheduler != nullptr) {
            return *scheduler;
        }
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->scheduler();
        }
        throw std::logic_error("Temporary draw connection requires an active runtime scene.");
    }
}

namespace MR {
    void connectToDrawTemporarily(NameObj *object) {
        require_scheduler(object).connect_draw(*object);
    }

    void disconnectToDrawTemporarily(NameObj *object) {
        require_scheduler(object).disconnect_draw(*object);
    }

    bool isConnectToDrawTemporarily(const NameObj *object) {
        auto &scheduler = require_scheduler(const_cast<NameObj *>(object));
        return scheduler.is_draw_connected(*object);
    }
}
