#include "Game/System/GameDataFunction.hpp"

namespace {
OSTime s_time_announced = 0;
}

namespace GameDataFunction {

OSTime getSysConfigFileTimeAnnounced() {
    return s_time_announced;
}

void updateSysConfigFileTimeAnnounced() {
    s_time_announced = OSGetTime();
}

}  // namespace GameDataFunction

