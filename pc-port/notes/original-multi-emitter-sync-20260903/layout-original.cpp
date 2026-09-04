#include "Game/Effect/SyncBckEffectInfo.hpp"
#include <stddef.h>

typedef char InfoSize[sizeof(SyncBckEffectInfo) == 0x18 ? 1 : -1];
typedef char ResourceSize[sizeof(SyncBckEffectInfo::BckResourceInfo) == 8 ? 1 : -1];
typedef char StartOffset[offsetof(SyncBckEffectInfo, mStartFrame) == 0xC ? 1 : -1];
typedef char EndOffset[offsetof(SyncBckEffectInfo, mEndFrame) == 0x10 ? 1 : -1];
typedef char ContinueOffset[offsetof(SyncBckEffectInfo, mContinueBckEnd) == 0x14 ? 1 : -1];
