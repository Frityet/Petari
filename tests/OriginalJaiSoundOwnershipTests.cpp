#include "JSystem/JAudio2/JAISound.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    static_assert(sizeof(JAISoundHandle) == sizeof(JAISound*));
    JAISoundID id(2, 13, 0x1234);
    assert(u32(id) == 0x020D1234 && id.getGroupID() == 13 && id.getWaveID() == 0x1234);
    JAISoundStatus_ status;
    status.init();
    status.pause(true);
    assert(status._0.value == 0x40);
    status._0.value = 0x80;
    assert(status.isMute() && !status.isPaused());
    status.setAnimationState(3);
    u8 flags = 0;
    std::memcpy(&flags, &status.mState.flags, sizeof(flags));
    assert(flags == 0x30 && status.getAnimationState() == 3);
    status.init();
    assert(status.lockWhenPrepared() == 1 && status.getState() == JAISoundStatus_::State_LOCK_PREPARE);
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_PREPARE);
    status.setReadyLocked();
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_READY);
    std::puts("[pass] original JAISoundID layout and JAISoundStatus flags/prepare transitions");
}
