#include "Game/Util/ObjUtil.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/SwitchEventFunctorListener.hpp"
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

    void listenNameObjStageSwitchOnAppear(const NameObj* pObj, const StageSwitchCtrl* pCtrl, const MR::FunctorBase& rOnFunctor) {
        SwitchEventFunctorListener* pListener = new SwitchEventFunctorListener();
        pListener->setOnFunctor(rOnFunctor);

        MR::getSwitchWatcherHolder()->joinSwitchEventListenerAppear(pCtrl, pListener);
    }

    void listenNameObjStageSwitchOnOffAppear(const NameObj* pObj, const StageSwitchCtrl* pCtrl, const MR::FunctorBase& rOnFunctor,
                                             const MR::FunctorBase& rOffFunctor) {
        SwitchEventFunctorListener* pListener = new SwitchEventFunctorListener();
        pListener->setOnFunctor(rOnFunctor);
        pListener->setOffFunctor(rOffFunctor);

        MR::getSwitchWatcherHolder()->joinSwitchEventListenerAppear(pCtrl, pListener);
    }

    void listenNameObjStageSwitchOnA(const NameObj* pObj, const StageSwitchCtrl* pCtrl, const MR::FunctorBase& rOnFunctor) {
        SwitchEventFunctorListener* pListener = new SwitchEventFunctorListener();
        pListener->setOnFunctor(rOnFunctor);

        MR::getSwitchWatcherHolder()->joinSwitchEventListenerA(pCtrl, pListener);
    }

    void listenNameObjStageSwitchOnOffA(const NameObj* pObj, const StageSwitchCtrl* pCtrl, const MR::FunctorBase& rOnFunctor,
                                        const MR::FunctorBase& rOffFunctor) {
        SwitchEventFunctorListener* pListener = new SwitchEventFunctorListener();
        pListener->setOnFunctor(rOnFunctor);
        pListener->setOffFunctor(rOffFunctor);

        MR::getSwitchWatcherHolder()->joinSwitchEventListenerA(pCtrl, pListener);
    }

    void listenNameObjStageSwitchOnB(const NameObj* pObj, const StageSwitchCtrl* pCtrl, const MR::FunctorBase& rOnFunctor) {
        SwitchEventFunctorListener* pListener = new SwitchEventFunctorListener();
        pListener->setOnFunctor(rOnFunctor);

        MR::getSwitchWatcherHolder()->joinSwitchEventListenerB(pCtrl, pListener);
    }

    void listenNameObjStageSwitchOffB(const NameObj* pObj, const StageSwitchCtrl* pCtrl, const MR::FunctorBase& rOffFunctor) {
        SwitchEventFunctorListener* pListener = new SwitchEventFunctorListener();
        pListener->setOffFunctor(rOffFunctor);

        MR::getSwitchWatcherHolder()->joinSwitchEventListenerB(pCtrl, pListener);
    }

    void listenNameObjStageSwitchOnOffB(const NameObj* pObj, const StageSwitchCtrl* pCtrl, const MR::FunctorBase& rOnFunctor,
                                        const MR::FunctorBase& rOffFunctor) {
        SwitchEventFunctorListener* pListener = new SwitchEventFunctorListener();
        pListener->setOnFunctor(rOnFunctor);
        pListener->setOffFunctor(rOffFunctor);

        MR::getSwitchWatcherHolder()->joinSwitchEventListenerB(pCtrl, pListener);
    }
}  // namespace MR
