#include "Game/Util/MessageUtil.hpp"

#include "Game/Util/LayoutUtil.hpp"

namespace MR {

bool isExistGameMessage(const char *pMessageName) {
    return getGameMessageDirect(pMessageName) != nullptr;
}

}  // namespace MR
