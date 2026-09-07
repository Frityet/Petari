#include "Game/Player/MarioFaint.hpp"
#include "Game/Player/MarioParalyze.hpp"
#include "Game/Player/MarioRecovery.hpp"
#include "Game/Player/MarioTeresa.hpp"
#include "Game/Player/MarioFoo.hpp"
#include "Game/Player/MarioHang.hpp"
#include "Game/Player/MarioWait.hpp"
#include <cstddef>
typedef char MarioFaintSize[(sizeof(MarioFaint) == 0x28) ? 1 : -1];
typedef char MarioParalyzeSize[(sizeof(MarioParalyze) == 0x1c) ? 1 : -1];
typedef char MarioRecoverySize[(sizeof(MarioRecovery) == 0x8c) ? 1 : -1];
typedef char MarioTeresaSize[(sizeof(MarioTeresa) == 0x5c) ? 1 : -1];
typedef char MarioFooSize[(sizeof(MarioFoo) == 0x6bc) ? 1 : -1];
typedef char MarioHangSize[(sizeof(MarioHang) == 0x44) ? 1 : -1];
typedef char MarioWaitSize[(sizeof(MarioWait) == 0x18) ? 1 : -1];
typedef char MarioFaint_18Offset[(offsetof(MarioFaint, _18) == 0x18) ? 1 : -1];
typedef char MarioParalyze_18Offset[(offsetof(MarioParalyze, _18) == 0x18) ? 1 : -1];
typedef char MarioRecovery_88Offset[(offsetof(MarioRecovery, _88) == 0x88) ? 1 : -1];
typedef char MarioTeresa_59Offset[(offsetof(MarioTeresa, _59) == 0x59) ? 1 : -1];
typedef char MarioFoo_6B8Offset[(offsetof(MarioFoo, _6B8) == 0x6b8) ? 1 : -1];
typedef char MarioHangmHangTimerOffset[(offsetof(MarioHang, mHangTimer) == 0x18) ? 1 : -1];
typedef char MarioHangmWallSensorOffset[(offsetof(MarioHang, mWallSensor) == 0x40) ? 1 : -1];
typedef char MarioWait_16Offset[(offsetof(MarioWait, _16) == 0x16) ? 1 : -1];
