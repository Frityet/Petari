#include "Game/Util/MessageUtil.hpp"

#include <string_view>

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    bool isExistGameMessage(const char* pMessageId) {
        if (pMessageId == nullptr) {
            return false;
        }
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            return runtime->messages().message(std::string_view(pMessageId)) != nullptr;
        }
        return false;
    }
}
