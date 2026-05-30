#include "Game/Util/ObjUtil.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/Functor.hpp"
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

    enum class StageSwitchSlot {
        A,
        B,
        Appear,
        Dead,
        Sleep,
    };

    struct StageSwitchRecord {
        s32 switch_a = -1;
        s32 switch_b = -1;
        s32 switch_appear = -1;
        s32 switch_dead = -1;
        s32 switch_sleep = -1;
    };

    struct StageSwitchListener {
        const LiveActor* actor = nullptr;
        s32 switch_id = -1;
        std::unique_ptr<MR::FunctorBase> on_functor{};
        std::unique_ptr<MR::FunctorBase> off_functor{};
    };

    [[nodiscard]] std::unordered_map<const LiveActor*, StageSwitchRecord>& stage_switch_records() {
        static auto records = std::unordered_map<const LiveActor*, StageSwitchRecord>{};
        return records;
    }

    [[nodiscard]] std::unordered_map<s32, bool>& stage_switch_states() {
        static auto states = std::unordered_map<s32, bool>{};
        return states;
    }

    [[nodiscard]] std::vector<StageSwitchListener>& stage_switch_listeners() {
        static auto listeners = std::vector<StageSwitchListener>{};
        return listeners;
    }

    [[nodiscard]] const char* actor_name(const LiveActor* actor) {
        return actor != nullptr ? actor->getName() : "";
    }

    [[nodiscard]] const char* switch_name(StageSwitchSlot slot) {
        switch (slot) {
        case StageSwitchSlot::A:
            return "A";
        case StageSwitchSlot::B:
            return "B";
        case StageSwitchSlot::Appear:
            return "APPEAR";
        case StageSwitchSlot::Dead:
            return "DEAD";
        case StageSwitchSlot::Sleep:
            return "SLEEP";
        }
        return "";
    }

    [[nodiscard]] const char* switch_field_name(StageSwitchSlot slot) {
        switch (slot) {
        case StageSwitchSlot::A:
            return "SW_A";
        case StageSwitchSlot::B:
            return "SW_B";
        case StageSwitchSlot::Appear:
            return "SW_APPEAR";
        case StageSwitchSlot::Dead:
            return "SW_DEAD";
        case StageSwitchSlot::Sleep:
            return "SW_SLEEP";
        }
        return "";
    }

    [[nodiscard]] const char* switch_event_name(StageSwitchSlot slot, std::string_view prefix) {
        if (prefix == "read") {
            switch (slot) {
            case StageSwitchSlot::A:
                return "read_a_registered";
            case StageSwitchSlot::B:
                return "read_b_registered";
            case StageSwitchSlot::Appear:
                return "read_appear_registered";
            case StageSwitchSlot::Dead:
                return "read_dead_registered";
            case StageSwitchSlot::Sleep:
                return "read_sleep_registered";
            }
        }
        if (prefix == "write") {
            switch (slot) {
            case StageSwitchSlot::A:
                return "write_a_registered";
            case StageSwitchSlot::B:
                return "write_b_registered";
            case StageSwitchSlot::Appear:
                return "write_appear_registered";
            case StageSwitchSlot::Dead:
                return "write_dead_registered";
            case StageSwitchSlot::Sleep:
                return "write_sleep_registered";
            }
        }
        if (prefix == "listener_on") {
            switch (slot) {
            case StageSwitchSlot::A:
                return "listener_on_a_registered";
            case StageSwitchSlot::B:
                return "listener_on_b_registered";
            case StageSwitchSlot::Appear:
                return "listener_on_appear_registered";
            case StageSwitchSlot::Dead:
                return "listener_on_dead_registered";
            case StageSwitchSlot::Sleep:
                return "listener_on_sleep_registered";
            }
        }
        if (prefix == "listener_off") {
            switch (slot) {
            case StageSwitchSlot::A:
                return "listener_off_a_registered";
            case StageSwitchSlot::B:
                return "listener_off_b_registered";
            case StageSwitchSlot::Appear:
                return "listener_off_appear_registered";
            case StageSwitchSlot::Dead:
                return "listener_off_dead_registered";
            case StageSwitchSlot::Sleep:
                return "listener_off_sleep_registered";
            }
        }
        return "registered";
    }

    [[nodiscard]] const char* switch_transition_event_name(StageSwitchSlot slot, bool state) {
        switch (slot) {
        case StageSwitchSlot::A:
            return state ? "switch_a_on" : "switch_a_off";
        case StageSwitchSlot::B:
            return state ? "switch_b_on" : "switch_b_off";
        case StageSwitchSlot::Appear:
            return state ? "switch_appear_on" : "switch_appear_off";
        case StageSwitchSlot::Dead:
            return state ? "switch_dead_on" : "switch_dead_off";
        case StageSwitchSlot::Sleep:
            return state ? "switch_sleep_on" : "switch_sleep_off";
        }
        return state ? "switch_on" : "switch_off";
    }

    void emit_stage_switch_trace(const char* event, const LiveActor* actor, StageSwitchSlot slot, s32 switch_id, bool state) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("stage_switch", event,
                                               "object=" + std::string(actor_name(actor)) + ";switch=" + switch_name(slot) + ";id=" +
                                                   std::to_string(switch_id) + ";state=" + (state ? "on" : "off"));
        }
#else
        (void)event;
        (void)actor;
        (void)slot;
        (void)switch_id;
        (void)state;
#endif
    }

    void emit_stage_switch_raw_trace(const char* event, s32 switch_id, bool state) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("stage_switch", event,
                                               "object=StageSwitchCtrl;switch=RAW;id=" + std::to_string(switch_id) +
                                                   ";state=" + (state ? "on" : "off"));
        }
#else
        (void)event;
        (void)switch_id;
        (void)state;
#endif
    }

    [[nodiscard]] s32& switch_id_ref(StageSwitchRecord& record, StageSwitchSlot slot) {
        switch (slot) {
        case StageSwitchSlot::A:
            return record.switch_a;
        case StageSwitchSlot::B:
            return record.switch_b;
        case StageSwitchSlot::Appear:
            return record.switch_appear;
        case StageSwitchSlot::Dead:
            return record.switch_dead;
        case StageSwitchSlot::Sleep:
            return record.switch_sleep;
        }
        return record.switch_a;
    }

    [[nodiscard]] s32 switch_id(const StageSwitchRecord& record, StageSwitchSlot slot) {
        switch (slot) {
        case StageSwitchSlot::A:
            return record.switch_a;
        case StageSwitchSlot::B:
            return record.switch_b;
        case StageSwitchSlot::Appear:
            return record.switch_appear;
        case StageSwitchSlot::Dead:
            return record.switch_dead;
        case StageSwitchSlot::Sleep:
            return record.switch_sleep;
        }
        return -1;
    }

    [[nodiscard]] s32 actor_switch_id(const LiveActor* actor, StageSwitchSlot slot) {
        const auto& records = stage_switch_records();
        const auto found = records.find(actor);
        return found != records.end() ? switch_id(found->second, slot) : -1;
    }

    [[nodiscard]] bool read_switch_id(const JMapInfoIter& rIter, StageSwitchSlot slot, s32* out) {
        if (out == nullptr) {
            return false;
        }

        auto id = s32{-1};
        if (!MR::isValidInfo(rIter) || !rIter.getValue<s32>(switch_field_name(slot), &id) || id < 0) {
            return false;
        }

        *out = id;
        return true;
    }

    bool register_stage_switch(LiveActor* actor, const JMapInfoIter& rIter, StageSwitchSlot slot, std::string_view access) {
        if (actor == nullptr) {
            return false;
        }

        auto id = s32{-1};
        if (!read_switch_id(rIter, slot, &id)) {
            return false;
        }

        auto& record = stage_switch_records()[actor];
        switch_id_ref(record, slot) = id;
        auto& state = stage_switch_states()[id];
        emit_stage_switch_trace(switch_event_name(slot, access), actor, slot, id, state);
        return true;
    }

    [[nodiscard]] bool is_valid_switch(const LiveActor* actor, StageSwitchSlot slot) {
        return actor_switch_id(actor, slot) >= 0;
    }

    [[nodiscard]] bool is_on_switch(const LiveActor* actor, StageSwitchSlot slot) {
        const auto id = actor_switch_id(actor, slot);
        if (id < 0) {
            return false;
        }

        const auto& states = stage_switch_states();
        const auto found = states.find(id);
        return found != states.end() && found->second;
    }

    void notify_switch_listeners(s32 id, bool state) {
        for (const auto& listener : stage_switch_listeners()) {
            if (listener.switch_id != id || listener.actor == nullptr) {
                continue;
            }

            if (state && listener.on_functor != nullptr) {
                (*listener.on_functor)();
            }
            else if (!state && listener.off_functor != nullptr) {
                (*listener.off_functor)();
            }
        }
    }

    void set_switch_state(LiveActor* actor, StageSwitchSlot slot, bool state) {
        const auto id = actor_switch_id(actor, slot);
        if (id < 0) {
            return;
        }

        auto& states = stage_switch_states();
        const auto was_on = states[id];
        states[id] = state;
        emit_stage_switch_trace(switch_transition_event_name(slot, state), actor, slot, id, state);
        if (was_on != state) {
            notify_switch_listeners(id, state);
        }
    }

    void set_raw_switch_state(s32 id, bool state) {
        if (id < 0) {
            return;
        }

        auto& states = stage_switch_states();
        const auto was_on = states[id];
        states[id] = state;
        emit_stage_switch_raw_trace(state ? "switch_raw_on" : "switch_raw_off", id, state);
        if (was_on != state) {
            notify_switch_listeners(id, state);
        }
    }

    void register_switch_listener(LiveActor* actor, StageSwitchSlot slot, const MR::FunctorBase* on_functor,
                                  const MR::FunctorBase* off_functor) {
        if (actor == nullptr || (on_functor == nullptr && off_functor == nullptr)) {
            return;
        }

        const auto id = actor_switch_id(actor, slot);
        if (id < 0) {
            return;
        }

        auto listener = StageSwitchListener{.actor = actor, .switch_id = id};
        if (on_functor != nullptr) {
            listener.on_functor.reset(on_functor->clone(nullptr));
            emit_stage_switch_trace(switch_event_name(slot, "listener_on"), actor, slot, id, is_on_switch(actor, slot));
        }
        if (off_functor != nullptr) {
            listener.off_functor.reset(off_functor->clone(nullptr));
            emit_stage_switch_trace(switch_event_name(slot, "listener_off"), actor, slot, id, is_on_switch(actor, slot));
        }

        stage_switch_listeners().push_back(std::move(listener));
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

    void connectToSceneAreaObj(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_AreaObj, -1, -1, -1);
    }

    void connectToSceneNpc(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_NPC, -1);
    }

    void connectToSceneNpcMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_NPC, -1, -1, -1);
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

    bool useStageSwitchReadA(LiveActor* pActor, const JMapInfoIter& rIter) {
        return register_stage_switch(pActor, rIter, StageSwitchSlot::A, "read");
    }

    bool useStageSwitchReadB(LiveActor* pActor, const JMapInfoIter& rIter) {
        return register_stage_switch(pActor, rIter, StageSwitchSlot::B, "read");
    }

    bool useStageSwitchReadAppear(LiveActor* pActor, const JMapInfoIter& rIter) {
        return register_stage_switch(pActor, rIter, StageSwitchSlot::Appear, "read");
    }

    void useStageSwitchSleep(LiveActor* pActor, const JMapInfoIter& rIter) {
        (void)register_stage_switch(pActor, rIter, StageSwitchSlot::Sleep, "read");
    }

    bool useStageSwitchWriteA(LiveActor* pActor, const JMapInfoIter& rIter) {
        return register_stage_switch(pActor, rIter, StageSwitchSlot::A, "write");
    }

    bool useStageSwitchWriteB(LiveActor* pActor, const JMapInfoIter& rIter) {
        return register_stage_switch(pActor, rIter, StageSwitchSlot::B, "write");
    }

    bool useStageSwitchWriteDead(LiveActor* pActor, const JMapInfoIter& rIter) {
        return register_stage_switch(pActor, rIter, StageSwitchSlot::Dead, "write");
    }

    bool needStageSwitchReadA(LiveActor* pActor, const JMapInfoIter& rIter) {
        return useStageSwitchReadA(pActor, rIter);
    }

    bool needStageSwitchReadB(LiveActor* pActor, const JMapInfoIter& rIter) {
        return useStageSwitchReadB(pActor, rIter);
    }

    bool needStageSwitchReadAppear(LiveActor* pActor, const JMapInfoIter& rIter) {
        return useStageSwitchReadAppear(pActor, rIter);
    }

    bool needStageSwitchWriteA(LiveActor* pActor, const JMapInfoIter& rIter) {
        return useStageSwitchWriteA(pActor, rIter);
    }

    bool needStageSwitchWriteB(LiveActor* pActor, const JMapInfoIter& rIter) {
        return useStageSwitchWriteB(pActor, rIter);
    }

    bool needStageSwitchWriteDead(LiveActor* pActor, const JMapInfoIter& rIter) {
        return useStageSwitchWriteDead(pActor, rIter);
    }

    bool isValidSwitchA(const LiveActor* pActor) {
        return is_valid_switch(pActor, StageSwitchSlot::A);
    }

    bool isValidSwitchB(const LiveActor* pActor) {
        return is_valid_switch(pActor, StageSwitchSlot::B);
    }

    bool isValidSwitchAppear(const LiveActor* pActor) {
        return is_valid_switch(pActor, StageSwitchSlot::Appear);
    }

    bool isValidSwitchDead(const LiveActor* pActor) {
        return is_valid_switch(pActor, StageSwitchSlot::Dead);
    }

    bool isOnSwitchA(const LiveActor* pActor) {
        return is_on_switch(pActor, StageSwitchSlot::A);
    }

    bool isOnSwitchB(const LiveActor* pActor) {
        return is_on_switch(pActor, StageSwitchSlot::B);
    }

    bool isOnSwitchAppear(const LiveActor* pActor) {
        return is_on_switch(pActor, StageSwitchSlot::Appear);
    }

    bool isOnStageSwitch(s32 switchId) {
        if (switchId < 0) {
            return false;
        }

        const auto& states = stage_switch_states();
        const auto found = states.find(switchId);
        return found != states.end() && found->second;
    }

    void onSwitchA(LiveActor* pActor) {
        set_switch_state(pActor, StageSwitchSlot::A, true);
    }

    void onSwitchB(LiveActor* pActor) {
        set_switch_state(pActor, StageSwitchSlot::B, true);
    }

    void onSwitchDead(LiveActor* pActor) {
        set_switch_state(pActor, StageSwitchSlot::Dead, true);
    }

    void onStageSwitchById(s32 switchId) {
        set_raw_switch_state(switchId, true);
    }

    void offSwitchA(LiveActor* pActor) {
        set_switch_state(pActor, StageSwitchSlot::A, false);
    }

    void offSwitchB(LiveActor* pActor) {
        set_switch_state(pActor, StageSwitchSlot::B, false);
    }

    void offSwitchDead(LiveActor* pActor) {
        set_switch_state(pActor, StageSwitchSlot::Dead, false);
    }

    void offStageSwitchById(s32 switchId) {
        set_raw_switch_state(switchId, false);
    }

    void listenStageSwitchOnAppear(LiveActor* pActor, const MR::FunctorBase& rFunctor) {
        register_switch_listener(pActor, StageSwitchSlot::Appear, &rFunctor, nullptr);
    }

    void listenStageSwitchOnOffAppear(LiveActor* pActor, const MR::FunctorBase& rFunctor1, const MR::FunctorBase& rFunctor2) {
        register_switch_listener(pActor, StageSwitchSlot::Appear, &rFunctor1, &rFunctor2);
    }

    void listenStageSwitchOnA(LiveActor* pActor, const MR::FunctorBase& rFunctor) {
        register_switch_listener(pActor, StageSwitchSlot::A, &rFunctor, nullptr);
    }

    void listenStageSwitchOnOffA(LiveActor* pActor, const MR::FunctorBase& rFunctor1, const MR::FunctorBase& rFunctor2) {
        register_switch_listener(pActor, StageSwitchSlot::A, &rFunctor1, &rFunctor2);
    }

    void listenStageSwitchOnB(LiveActor* pActor, const MR::FunctorBase& rFunctor) {
        register_switch_listener(pActor, StageSwitchSlot::B, &rFunctor, nullptr);
    }

    void listenStageSwitchOffB(LiveActor* pActor, const MR::FunctorBase& rFunctor) {
        register_switch_listener(pActor, StageSwitchSlot::B, nullptr, &rFunctor);
    }

    void listenStageSwitchOnOffB(LiveActor* pActor, const MR::FunctorBase& rFunctor1, const MR::FunctorBase& rFunctor2) {
        register_switch_listener(pActor, StageSwitchSlot::B, &rFunctor1, &rFunctor2);
    }
}  // namespace MR
