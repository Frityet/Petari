#pragma once

// The retail GameDataConst header names the original embedded BCSV type.  The
// PC JMapInfo implementation does not expose that storage type, so keep the
// declaration opaque while compiling the unchanged retail executor.
struct JMapData;

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/GameSceneFunction.hpp"
#include "Game/Screen/MoviePlayingSequence.hpp"
#include "Game/Screen/StaffRoll.hpp"
#include "Game/System/GalaxyMoveArgument.hpp"
#include "Game/System/GameDataConst.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/GameSystemFunction.hpp"
#include "Game/System/StageResultSequenceChecker.hpp"
#include "Game/System/StorySequenceExecutor.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/StringUtil.hpp"

#include <cstdio>

namespace GameSequenceFunction {
    void activateGalaxyCometScheduler();
    void deactivateGalaxyCometScheduler();
    bool hasStageResultSequence();
    bool hasPowerStarYetAtResultSequence();
    bool isLuigiDisappearFromAstroGalaxy();
    const char* getClearedStageName();
    s32 getClearedPowerStarId();
}  // namespace GameSequenceFunction

namespace GameDataFunction {
    bool isDataMario();
    bool hasPowerStar(const char*, s32);
    bool hasGrandStar(int);
    bool canOnGameEventFlag(const char*);
    bool canOnAndIsOffGameEventFlag(const char*);
    bool isOnJustGameEventFlag(const char*);
    bool canOnJustGameEventFlag(const char*);
}  // namespace GameDataFunction

namespace MR {
    const char* getCurrentStageName();
    s32 getCurrentScenarioNo();
    bool isEqualSceneName(const char*);
    bool isStarCompleteAllGalaxy();
    void startStarPointerModeEnding(void*);
    void openWipeFade(s32 = -1);
    void closeWipeFade(s32 = -1);
}  // namespace MR
