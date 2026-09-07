#include "Game/Demo/DemoStartRequestUtil.hpp"

namespace DemoStartRequestUtil {
    bool isEmpty(const DemoStartInfo* pInfo) {
        if (pInfo->_0 != nullptr) {
            return false;
        }

        if (pInfo->_4 != nullptr) {
            return false;
        }

        if (pInfo->_8 != nullptr) {
            return false;
        }

        return pInfo->_C == nullptr;
    }
};  // namespace DemoStartRequestUtil
