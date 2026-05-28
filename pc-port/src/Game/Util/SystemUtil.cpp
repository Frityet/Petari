#include "Game/Util/SystemUtil.hpp"

#include "runtime/RuntimeContext.hpp"

namespace MR {

    bool isDisplayEncouragePal60Window() {
        return false;
    }

    void tryRumblePadMiddle(const NerveExecutor*, int channel) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->rumble().request_middle(channel);
        }
    }

}  // namespace MR
