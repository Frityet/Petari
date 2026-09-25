#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Camera/DotCamParams.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "runtime/RuntimeServices.hpp"
#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void verify() {
    auto* root = MR::getStageDataHolder();
    auto* archives = SingletonHolder<FileLoader>::get();
    smgpc::runtime::DvdFileSystemService dvd("/");
    require(root && archives, "Actual process owns both the stage and mounted archives");
    std::vector<const JMapInfo*> starts;
    const auto collect = [&](auto&& self, const StageDataHolder& stage) -> void {
        for (const auto& table : stage.mStartObjs) starts.push_back(&table);
        for (s32 i = 0; i < stage.mStageDataHolderCount; ++i) self(self, *stage.mStageDataArray[i]);
    };
    collect(collect, *root);
    s32 start_count = 0;
    for (const auto* table : starts) {
        for (s32 row = 0; row < table->getNumEntries(); ++row) {
            s32 camera = -1;
            require(table->getValue(row, "Camera_id", &camera), "Retail start row contains Camera_id");
            JMapIdInfo actual(-1, -1), expected(camera, JMapInfoIter(table, row));
            MR::getStartCameraIdInfoFromStartDataIndex(&actual, start_count++);
            require(actual._0 == expected._0 && actual.mZoneID == expected.mZoneID,
                    "Global start query preserves original recursive table and row order");
        }
    }
    require(start_count > 0 && MR::getStartPosNum() == start_count, "All actual stage start rows are reachable");

    unsigned camera_rows = 0, camera_rails = 0;
    for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
        auto* holder = root->getStageDataHolderFromZoneId(zone);
        if (!holder) continue;
        const auto archive = dvd.retain_archive_for_path(std::string("/StageData/") + holder->_A8 + ".arc");
        void* data = nullptr;
        s32 size = 0;
        MR::getStageCameraData(&data, &size, zone);
        auto* mounted_data = holder->getStageArchiveResource("CameraParam.bcam");
        require(data == mounted_data && size == holder->getStageArchiveResourceSize(mounted_data),
                "Original stage camera query keeps its actual StageDataHolder archive identity and size");
        if (archive->find_resource("CameraParam.bcam")) {
            const auto bytes = archive->resource_data("CameraParam.bcam");
            require(data != nullptr && size == bytes.size() && std::memcmp(data, bytes.data(), bytes.size()) == 0,
                    "Original mounted camera resource matches the independently read retail archive bytes");
            const auto expected = smgpc::resource::BcsvTable::from_bytes(bytes);
            std::vector<const char*> names;
            {
                DotCamReaderInBin reader(data);
                for (; reader.hasMoreChunk(); reader.nextToChunk()) {
                    const char* name = nullptr;
                    require(reader.getValueString("id", &name), "Original binary reader exposes every camera ID");
                    names.push_back(name);
                }
            }
            require(names.size() == expected.entry_count(), "Original reader traverses the complete retail camera table");
            for (std::size_t row = 0; row < names.size(); ++row)
                require(std::string_view(names[row]) == expected.get_string(row, "id"), "Borrowed original camera IDs survive reader retirement");
            camera_rows += names.size();
        } else {
            require(data == nullptr && size == -1, "Mounted archive's absent-resource size remains the original sentinel");
        }
        const auto* paths = holder->findJmpInfoFromArray(&holder->mPathObjs, "CommonPathInfo");
        if (!paths) continue;
        require(MR::getPlacedRailNum(zone) == paths->getNumEntries(), "Rail count matches original holder's authored path table");
        for (s32 row = 0; row < paths->getNumEntries(); ++row) {
            JMapInfoIter path;
            const JMapInfo* points = nullptr;
            const bool camera = MR::getCameraRailInfoFromRailDataIndex(&path, &points, row, zone);
            const char* usage = nullptr;
            require(path.mInfo == paths && path.mIndex == row && path.getValue("usage", &usage), "Rail query retains exact original row identity");
            require(camera == (std::strcmp(usage, "Camera") == 0), "Camera rail filtering follows the authored usage field");
            s32 id = -1;
            require(path.getValue("l_id", &id), "Original rail has its authored link ID");
            JMapInfoIter by_id;
            const JMapInfo* by_id_points = nullptr;
            MR::getCameraRailInfo(&by_id, &by_id_points, id, zone);
            require(by_id.mInfo == path.mInfo && by_id.mIndex == path.mIndex && by_id_points == points,
                    "Rail ID and row queries use the same original path and point tables");
            camera_rails += camera;
        }
    }
    require(camera_rows > 0, "Retail Gateway camera rows were checked");
    void* data = reinterpret_cast<void*>(1);
    s32 size = -1;
    MR::getStageCameraData(&data, &size, -1);
    require(data == nullptr && size == 0, "Unplaced zones preserve original null and zero results");
    MR::getCurrentScenarioStartAnimCameraData(&data, &size);
    char name[64];
    std::snprintf(name, sizeof(name), "StartScenario%d.canm", MR::getCurrentScenarioNo());
    auto* expected = root->getStageArchiveResource(name);
    require(data == expected && size == (expected ? root->getStageArchiveResourceSize(expected) : 0),
            "Scenario camera query uses the original stage archive resource and size");
    std::fprintf(stderr, "[stage-resources] starts=%d camera_rows=%u camera_rails=%u\n", start_count, camera_rows, camera_rails);
}
}
int main() { return smgpc::test::run_stage_resource_process("stage-camera-resources", verify); }
