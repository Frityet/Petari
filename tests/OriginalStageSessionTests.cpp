#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"
#include "Game/System/GameSequenceDirector.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/LiveActor/RailRider.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool same_id(const JMapIdInfo& left, const JMapIdInfo& right) {
    return left._0 == right._0 && left.mZoneID == right.mZoneID;
}

void verify_original_session(std::weak_ptr<const void>& zone_lifetime) {
    auto* system = SingletonHolder<GameSystem>::get();
    auto* controller = system->mSceneController;
    auto* parser = ScenarioDataFunction::getScenarioDataParser();
    auto* temporary = system->mSequenceDirector->mGameDataTemporaryInGalaxy;
    require(controller && parser == controller->mScenarioParser && temporary,
            "the actual scene controller and sequence own scenario and temporary data");
    require(MR::isEqualSceneName("Game") && MR::isEqualStageName("HeavensDoorGalaxy") &&
                std::string_view(MR::getCurrentStageName()) == controller->mCurrSceneControlInfo.mStage &&
                MR::getCurrentScenarioNo() == 1 &&
                MR::getCurrentSelectedScenarioNo() == controller->getCurrentSelectedScenarioNo(),
            "public stage selection reads the actual completed original scene controller");
    const auto* gateway = parser->getScenarioData("HeavensDoorGalaxy");
    require(gateway && gateway->getZoneNum() == 7, "the authored Gateway catalog retains its seven ZoneList rows");
    const auto accessor = MR::makeCurrentGalaxyStatusAccessor();
    require(accessor.mScenarioData == gateway, "current galaxy queries use the actual scene controller's parser");
    zone_lifetime = gateway->mZoneList->mResourceOwner;
    for (s32 zone = 0; zone < gateway->getZoneNum(); ++zone) {
        const char* name = gateway->getZoneName(zone);
        require(name && accessor.getZoneId(name) == zone &&
                    std::string_view(MR::getZoneNameFromZoneId(zone)) == name,
                "zone IDs and original public names round-trip the authored ZoneList order");
        std::string lower(name);
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
        require(gateway->getZoneId(lower.c_str()) == zone,
                "original zone lookup preserves case-insensitive authored name matching");
    }

    auto* restart = MR::getPlayerRestartIdInfo();
    auto* selected_start = controller->mCurrSceneControlInfo.mStartIdInfo;
    require(restart == temporary->mPlayerRestartIdInfo && selected_start && restart != selected_start,
            "restart accessor exposes temporary data independently of the selected scene-entry ID");
    const JMapIdInfo original_restart = *restart;
    const JMapIdInfo original_selected_start = *selected_start;
    const JMapIdInfo initial = MR::getInitializeStartIdInfo();
    struct RestoreRestart {
        GameDataTemporaryInGalaxy& data;
        JMapIdInfo saved;
        ~RestoreRestart() { data.setPlayerRestartIdInfo(saved); }
    } restore{*temporary, original_restart};
    require(initial._0 == 0 && initial.mZoneID == 0, "the original immutable initialization ID remains (0, 0)");
    const JMapIdInfo checkpoint(3, gateway->getZoneNum() - 1);
    MR::setPlayerRestartIdInfo(checkpoint);
    require(MR::getPlayerRestartIdInfo() == restart && same_id(*restart, checkpoint) &&
                same_id(*selected_start, original_selected_start) && same_id(MR::getInitializeStartIdInfo(), initial),
            "public checkpoint updates preserve temporary identity, authored zone and separate scene-entry/default IDs");
    temporary->resetPlayerRestartIdInfo();
    require(same_id(*MR::getPlayerRestartIdInfo(), initial) && same_id(*selected_start, original_selected_start),
            "original restart reset copies the immutable default without changing scene-entry selection");

    const char* comet = nullptr;
    require(gateway->getValueString("Comet", 1, &comet) && comet == nullptr &&
                accessor.getCometName(1) == nullptr && !MR::isGalaxyAnyCometAppearInCurrentStage(),
            "the actual Gateway empty Comet field yields no current original comet event");
    const auto* file_select = parser->getScenarioData("FileSelect");
    require(file_select && file_select->mScenarioData->searchItemInfo("Comet") < 0 &&
                GalaxyStatusAccessor(file_select).getCometName(1) == nullptr,
            "the authored FileSelect schema without a Comet field retains the original absent-name result");
    bool found_purple = false;
    for (s32 galaxy = 0; galaxy < parser->mScenarioData.size(); ++galaxy) {
        const auto* data = parser->getScenarioData(galaxy);
        const GalaxyStatusAccessor other(data);
        for (s32 scenario = 1; scenario <= data->getScenarioNum(); ++scenario) {
            const char* name = other.getCometName(scenario);
            if (name && std::string_view(name) == "Purple") {
                require(other.isValidCoin100(scenario) && other.isCometStar(scenario),
                        "authored Purple rows retain the original coin-100 and comet-star classification");
                found_purple = true;
            }
        }
    }
    require(found_purple && MR::getCurrentScenarioNo() == 1 && !MR::isGalaxyAnyCometAppearInCurrentStage(),
            "querying actual Purple metadata never replaces the selected Gateway scenario or comet state");

    unsigned rails = 0;
    auto* stage = MR::getStageDataHolder();
    for (int zone = 0; zone < gateway->getZoneNum(); ++zone) {
        const auto* placed = stage->getStageDataHolderFromZoneId(zone);
        if (!placed) continue;
        for (const auto& table : placed->mPlacementObjs) {
            for (int row = 0; row < table.getNumEntries(); ++row) {
                s32 rail_id = -1;
                if (!table.getValue(row, "CommonPath_ID", &rail_id) || rail_id < 0) continue;
                const JMapInfoIter placement(&table, row);
                const JMapInfo* points = nullptr;
                JMapInfoIter path;
                MR::getRailInfo(&path, &points, placement);
                require(path.isValid() && points && points->getNumEntries() >= 2,
                        "the original stage holder resolves authored placement rails and their point tables");
                RailRider rider(placement);
                require(rider.mBezierRail->mInfo == points && std::isfinite(rider.getTotalLength()) && rider.getTotalLength() > 0,
                        "the original RailRider consumes the actual stage resource table");
                const float speed = std::min(1.0F, rider.getTotalLength() / 4.0F);
                rider.setSpeed(speed);
                rider.move();
                require(std::abs(rider.mCoord - speed) < 0.001F && std::isfinite(rider.mCurPos.x) &&
                            std::isfinite(rider.mCurPos.y) && std::isfinite(rider.mCurPos.z),
                        "original rail movement advances by world-space distance using the resource's float coordinates");
                ++rails;
            }
        }
    }
    require(rails > 0, "the real Gateway fixture must exercise at least one authored placement rail");
    std::fprintf(stderr, "[jmap-probe] Original stage lookup and RailRider movement passed for %u placements\n", rails);
}
}

int main() try {
    const auto registered_before = NameObj::snapshotNativeObjects().size();
    std::weak_ptr<const void> zone_lifetime;
    const int result = smgpc::test::run_stage_resource_process("original-stage-session", [&] {
        verify_original_session(zone_lifetime);
    });
    if (result != 0) return result;
    require(!SingletonHolder<GameSystem>::get() && zone_lifetime.expired() &&
                NameObj::snapshotNativeObjects().size() == registered_before,
            "normal original process retirement clears the actual session owners and scenario metadata");
    std::puts("PASS original stage selection, restart ownership, authored zones/comets and normal retirement");
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL original stage session: %s\n", error.what());
    return 1;
}
