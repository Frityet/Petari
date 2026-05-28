#include "Game/Util/ObjUtil.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {
    [[nodiscard]] bool ends_with(std::string_view text, std::string_view suffix) {
        return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }
        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] std::string archive_file_name(std::string_view archiveName) {
        auto name = base_name(archiveName);
        if (!ends_with(lower_copy(name), ".arc")) {
            name.append(".arc");
        }
        return name;
    }
}  // namespace

namespace MR {
    void requestMovementOn(NameObj* pObj) {
        NameObjFunction::requestMovementOn(pObj);
    }

    void requestMovementOff(NameObj* pObj) {
        NameObjFunction::requestMovementOff(pObj);
    }

    void connectToSceneMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObj, -1);
    }

    void connectToSceneMapObjMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_MapObj, -1, -1, -1);
    }

    void connectToSceneNpc(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_NPC, -1);
    }

    void connectToSceneLayout(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_Layout, MR::CalcAnimType_Layout, MR::DrawType_Layout);
        }
    }

    void connectToSceneLayoutDecoration(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_LayoutDecoration, MR::CalcAnimType_LayoutDecoration,
                                           MR::DrawType_LayoutDecoration);
        }
    }

    void connectToSceneTalkLayout(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_Layout, MR::CalcAnimType_Layout, MR::DrawType_TalkLayout);
        }
    }

    void connectToSceneLayoutOnPause(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_LayoutOnPause, MR::CalcAnimType_Layout, MR::DrawType_LayoutOnPause);
        }
    }

    bool isExistResourceInArc(const char* pArcName, const char* pResourceName) {
        if (pArcName == nullptr || pResourceName == nullptr) {
            return false;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return false;
        }

        const auto archive = archive_file_name(pArcName);
        const auto archive_path = runtime->dvd().find_first({
            std::filesystem::path(pArcName),
            std::filesystem::path("KrKorean") / "LayoutData" / archive,
            std::filesystem::path("LayoutData") / archive,
            std::filesystem::path("ObjectData") / archive,
        });
        if (!archive_path.has_value()) {
            return false;
        }

        const auto& rarc = runtime->dvd().archive_for_path(*archive_path);
        if (rarc.contains(pResourceName)) {
            return true;
        }

        const auto requested_name = lower_copy(base_name(pResourceName));
        return std::ranges::any_of(rarc.entries(), [&requested_name](const auto& entry) { return lower_copy(base_name(entry.path)) == requested_name; });
    }

    bool tryRumblePadStrong(const void*, s32 channel) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->rumble().request_strong(channel);
        }
        return true;
    }

    bool tryRumblePadWeak(const void*, s32 channel) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->rumble().request_weak(channel);
        }
        return true;
    }

    void shakeCameraNormal() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->camera_system().request_normal_shake();
        }
    }

    bool useStageSwitchWriteA(LiveActor*, const JMapInfoIter& rIter) {
        return MR::isValidInfo(rIter) && MR::isExistStageSwitchA(rIter);
    }

    void onSwitchA(LiveActor*) {
    }

    void offSwitchA(LiveActor*) {
    }
}  // namespace MR
