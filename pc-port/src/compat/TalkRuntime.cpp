#include "compat/TalkRuntime.hpp"

#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/AreaObj/MessageArea.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkMessageFunc.hpp"
#include "Game/NPC/TalkMessageInfo.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <limits>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

    smgpc::compat::TalkRuntime* sCurrentTalkRuntime = nullptr;

    [[nodiscard]] std::wstring to_wide(std::u16string_view value) {
        auto result = std::wstring{};
        result.reserve(value.size());
        for (const auto code : value) {
            result.push_back(static_cast<wchar_t>(code));
        }
        return result;
    }

    [[nodiscard]] std::u16string to_utf16(const wchar_t* value) {
        auto result = std::u16string{};
        if (value == nullptr) {
            return result;
        }
        while (*value != L'\0') {
            result.push_back(static_cast<char16_t>(*value++));
        }
        return result;
    }

    [[nodiscard]] bool event_node_continues(const TalkNode* node) {
        return node != nullptr && node->mNodeType == 3U && node->mGroupID <= 4U &&
               ((1U << node->mGroupID) & 0x15U) != 0U;
    }

}  // namespace

namespace smgpc::compat {

    struct TalkRuntime::Impl {
        struct CallbackAllocation {
            TalkMessageFuncBase* value = nullptr;

            CallbackAllocation() = default;
            CallbackAllocation(const CallbackAllocation&) = delete;
            CallbackAllocation& operator=(const CallbackAllocation&) = delete;

            CallbackAllocation(CallbackAllocation&& other) noexcept
                : value(std::exchange(other.value, nullptr)) {
            }

            CallbackAllocation& operator=(CallbackAllocation&& other) noexcept {
                if (this != &other) {
                    reset();
                    value = std::exchange(other.value, nullptr);
                }
                return *this;
            }

            ~CallbackAllocation() {
                reset();
            }

            void reset(TalkMessageFuncBase* replacement = nullptr) {
                if (value != nullptr) {
                    // The retail callback family has no virtual destructor.
                    // All source TalkMessageFuncM specializations contain only
                    // pointer/member-pointer values and are trivially
                    // destructible, so reclaim their clone allocation without
                    // changing the Game vtable.
                    value->~TalkMessageFuncBase();
                    ::operator delete(static_cast<void*>(value));
                }
                value = replacement;
            }
        };

        struct ControllerState {
            std::unique_ptr<TalkNodeCtrl> node_ctrl;
            std::unique_ptr<ActorCameraInfo> camera_info;
            std::string flow_key;
            std::wstring current_message;
            std::optional<std::uint32_t> direct_message_index;
            std::uint64_t last_request_tick = 0U;
            bool start_latch = false;
            bool end_latch = false;
            CallbackAllocation branch_func;
            CallbackAllocation event_func;
            CallbackAllocation anime_func;
            CallbackAllocation kill_func;
        };

        explicit Impl(TalkRuntime& runtime) : owner(runtime) {
        }

        TalkRuntime& owner;
        bool bound = false;
        bool nodes_loaded = false;
        std::uint64_t tick = 0U;
        std::vector<TalkNode> nodes;
        std::unordered_map<TalkMessageCtrl*, ControllerState> controllers;
        std::unordered_map<const TalkNodeCtrl*, TalkMessageCtrl*> controller_by_node;
        std::unordered_map<const LiveActor*, std::unique_ptr<TalkMessageCtrl>> owned_controllers;
        TalkMessageCtrl* pending = nullptr;
        TalkMessageCtrl* selected = nullptr;
        TalkMessageCtrl* active = nullptr;
        std::optional<TalkPresentation> presentation;
        bool active_force = false;
        bool active_time_keep_pause = false;
        bool input_released_after_open = true;

        [[nodiscard]] smgpc::runtime::RuntimeContext& runtime_context() const {
            auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
            if (runtime == nullptr) {
                throw std::logic_error("TalkRuntime requires an active RuntimeContext.");
            }
            return *runtime;
        }

        [[nodiscard]] smgpc::runtime::MessageService& messages() const {
            return runtime_context().messages();
        }

        [[nodiscard]] ControllerState& state(TalkMessageCtrl& controller) {
            const auto found = controllers.find(&controller);
            if (found == controllers.end()) {
                throw std::logic_error("TalkMessageCtrl is not registered with the active scene TalkRuntime.");
            }
            return found->second;
        }

        [[nodiscard]] const ControllerState& state(const TalkMessageCtrl& controller) const {
            const auto found = controllers.find(const_cast<TalkMessageCtrl*>(&controller));
            if (found == controllers.end()) {
                throw std::logic_error("TalkMessageCtrl is not registered with the active scene TalkRuntime.");
            }
            return found->second;
        }

        [[nodiscard]] TalkMessageCtrl& controller(TalkNodeCtrl& node_ctrl) {
            const auto found = controller_by_node.find(&node_ctrl);
            if (found == controller_by_node.end()) {
                throw std::logic_error("TalkNodeCtrl is not registered with the active scene TalkRuntime.");
            }
            return *found->second;
        }

        [[nodiscard]] const TalkMessageCtrl& controller(const TalkNodeCtrl& node_ctrl) const {
            const auto found = controller_by_node.find(&node_ctrl);
            if (found == controller_by_node.end()) {
                throw std::logic_error("TalkNodeCtrl is not registered with the active scene TalkRuntime.");
            }
            return *found->second;
        }

        void ensure_nodes_loaded() {
            if (nodes_loaded) {
                return;
            }
            nodes_loaded = true;
            const auto* flow = messages().flow_data();
            if (flow == nullptr) {
                return;
            }
            nodes.reserve(flow->nodes.size());
            for (const auto& source : flow->nodes) {
                auto node = TalkNode{};
                node.mNodeType = source.node_type;
                node.mGroupID = source.group_id;
                node.mIndex = source.index;
                node.mNextIdx = source.next_index;
                node.mNextGroup = source.next_group;
                nodes.push_back(node);
            }
        }

        [[nodiscard]] TalkNode* node(std::uint32_t index) {
            ensure_nodes_loaded();
            return index < nodes.size() ? &nodes[index] : nullptr;
        }

        [[nodiscard]] const TalkNode* node(std::uint32_t index) const {
            return index < nodes.size() ? &nodes[index] : nullptr;
        }

        [[nodiscard]] std::optional<std::uint32_t> node_index(const TalkNode* wanted) const {
            if (wanted == nullptr) {
                return std::nullopt;
            }
            for (auto index = std::size_t{}; index < nodes.size(); ++index) {
                if (&nodes[index] == wanted) {
                    return static_cast<std::uint32_t>(index);
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] TalkNode* branch_node(std::uint32_t branch_index) {
            const auto target = messages().branch_flow_node(branch_index);
            return target.has_value() ? node(*target) : nullptr;
        }

        [[nodiscard]] TalkNode* next_node(const TalkNodeCtrl& node_ctrl) {
            const auto* current = node_ctrl.mCurrentNode;
            if (current == nullptr) {
                return nullptr;
            }
            if (current->mNodeType == 1U) {
                if (current->mNextIdx == 0xffffU) {
                    return nullptr;
                }
                auto* result = node(current->mNextIdx);
                if (result == nullptr) {
                    throw std::logic_error("Talk message node points outside the retained FLW node table.");
                }
                return result;
            }
            if (current->mNodeType == 3U) {
                const auto* flow = messages().flow_data();
                if (flow == nullptr || current->mIndex >= flow->branch_node_indices.size()) {
                    throw std::logic_error("Talk event node points outside the retained FLW branch table.");
                }
                if (flow->branch_node_indices[current->mIndex] == 0xffffU) {
                    return nullptr;
                }
                auto* result = branch_node(current->mIndex);
                if (result == nullptr) {
                    throw std::logic_error("Talk event branch target is outside the retained FLW node table.");
                }
                return result;
            }
            if (current->mNodeType != 2U) {
                throw std::logic_error("Talk flow contains an unknown node type.");
            }
            return nullptr;
        }

        void set_message_info(TalkMessageCtrl& controller, std::uint32_t message_index) {
            auto& controller_state = state(controller);
            auto& node_ctrl = *controller_state.node_ctrl;
            const auto* message_id = messages().message_id(message_index);
            if (message_id == nullptr) {
                throw std::logic_error("Talk flow refers to a message index outside MessageId.tbl.");
            }
            const auto* raw_text = messages().message_raw_utf16(*message_id);
            const auto* info = messages().message_info(*message_id);
            if (raw_text == nullptr || info == nullptr) {
                throw std::logic_error("Talk flow message metadata is unavailable.");
            }

            controller_state.current_message = to_wide(*raw_text);
            node_ctrl.mMessageInfo = TalkMessageInfo{};
            node_ctrl.mMessageInfo._0 = reinterpret_cast<u8*>(controller_state.current_message.data());
            node_ctrl.mMessageInfo.mCameraSetID = info->camera_set_id;
            node_ctrl.mMessageInfo._6 = static_cast<s8>(info->unknown_06);
            node_ctrl.mMessageInfo.mCameraType = info->camera_type;
            node_ctrl.mMessageInfo.mTalkType = info->talk_type;
            node_ctrl.mMessageInfo.mBalloonType = info->balloon_type;
            node_ctrl.mMessageInfo._A = static_cast<s8>(info->unknown_0a);
            node_ctrl.mMessageInfo._B = static_cast<s8>(info->unknown_0b);
            node_ctrl.mCurrentNodeIdx = static_cast<s32>(message_index);

            const auto* next = next_node(node_ctrl);
            node_ctrl.mNodeData = next != nullptr && next->mNodeType == 2U
                                      ? static_cast<s16>(next->mIndex)
                                      : static_cast<s16>(-1);

            if (node_ctrl.mMessageInfo.isEventTalk() &&
                node_ctrl.mHistory.search(static_cast<u16>(message_index))) {
                node_ctrl.mMessageInfo.mTalkType = 0U;
            }
            if (node_ctrl.mMessageInfo.isEventTalk() && node_ctrl.mMessageInfo._B != -1 &&
                MR::isOnMessageAlreadyRead(node_ctrl.mMessageInfo._B)) {
                node_ctrl.mMessageInfo.mTalkType = 0U;
            }
        }

        void update_message(TalkNodeCtrl& node_ctrl) {
            auto& talk_controller = controller(node_ctrl);
            if (node_ctrl.mCurrentNode == nullptr) {
                if (const auto direct = state(talk_controller).direct_message_index; direct.has_value()) {
                    set_message_info(talk_controller, *direct);
                }
                return;
            }
            if (node_ctrl.mCurrentNode->mNodeType != 1U) {
                node_ctrl.mMessageInfo._0 = nullptr;
                return;
            }
            set_message_info(talk_controller, node_ctrl.mCurrentNode->mIndex);
        }

        void register_controller(TalkMessageCtrl& controller) {
            if (controllers.contains(&controller)) {
                throw std::logic_error("TalkMessageCtrl was registered twice with one TalkRuntime.");
            }
            auto controller_state = ControllerState{};
            controller_state.node_ctrl = std::make_unique<TalkNodeCtrl>();
            auto [found, inserted] = controllers.emplace(&controller, std::move(controller_state));
            static_cast<void>(inserted);
            controller.mNodeCtrl = found->second.node_ctrl.get();
            controller_by_node.emplace(controller.mNodeCtrl, &controller);
        }

        void resume_time_keep_if_needed() {
            if (!active_time_keep_pause || active == nullptr) {
                active_time_keep_pause = false;
                return;
            }
            if (auto* demo = active_demo_scene_runtime(); demo != nullptr) {
                demo->resume_time_keep(active->mHostActor);
            }
            active_time_keep_pause = false;
            if (presentation.has_value()) {
                presentation->time_keep_paused = false;
            }
        }

        void unregister_controller(TalkMessageCtrl& controller) {
            const auto found = controllers.find(&controller);
            if (found == controllers.end()) {
                return;
            }
            if (active == &controller) {
                resume_time_keep_if_needed();
                active = nullptr;
                presentation.reset();
            }
            if (pending == &controller) {
                pending = nullptr;
            }
            if (selected == &controller) {
                selected = nullptr;
            }
            controller_by_node.erase(found->second.node_ctrl.get());
            controller.mNodeCtrl = nullptr;
            controller.mCameraInfo = nullptr;
            controller.mBranchFunc = nullptr;
            controller.mEventFunc = nullptr;
            controller.mAnimeFunc = nullptr;
            controller.mKillFunc = nullptr;
            controllers.erase(found);
        }

        void create_message_direct(TalkMessageCtrl& controller, const JMapInfoIter& iter,
                                   std::string_view flow_key, ActorCameraInfo** camera_info) {
            if (flow_key.empty()) {
                throw std::logic_error("Talk flow keys must not be empty.");
            }
            auto& controller_state = state(controller);
            auto& node_ctrl = *controller_state.node_ctrl;
            controller_state.flow_key = std::string(flow_key);
            controller_state.direct_message_index.reset();
            node_ctrl._0 = controller_state.flow_key.data();

            const auto message_index = messages().message_index(flow_key);
            if (!message_index.has_value()) {
                throw std::logic_error("Talk flow key '" + std::string(flow_key) + "' is absent from MessageId.tbl.");
            }

            const auto root_index = messages().first_flow_node_for_message(*message_index);
            if (root_index.has_value()) {
                auto* root = node(*root_index);
                if (root == nullptr) {
                    throw std::logic_error("Talk flow root is outside the retained FLW node table.");
                }
                node_ctrl._38 = root;
                node_ctrl.mCurrentNode = root;
                node_ctrl.mFlowNode = root;
                update_message(node_ctrl);
            } else {
                node_ctrl._38 = nullptr;
                node_ctrl.mCurrentNode = nullptr;
                node_ctrl.mFlowNode = nullptr;
                controller_state.direct_message_index = *message_index;
                set_message_info(controller, *message_index);
            }

            if (node_ctrl.mMessageInfo.isFlowTalk()) {
                node_ctrl.forwardFlowNode();
                node_ctrl._38 = node_ctrl.mCurrentNode;
                node_ctrl.mFlowNode = node_ctrl.mCurrentNode;
            }

            controller_state.camera_info = iter.isValid()
                                               ? std::make_unique<ActorCameraInfo>(iter)
                                               : std::make_unique<ActorCameraInfo>(0, 0);
            controller.mCameraInfo = controller_state.camera_info.get();
            if (camera_info != nullptr) {
                *camera_info = controller.mCameraInfo;
            }
            node_ctrl.resetFlowNode();
        }

        [[nodiscard]] std::vector<smgpc::resource::BmgFormatArg>
        format_args(const TalkMessageCtrl& controller) const {
            auto result = std::vector<smgpc::resource::BmgFormatArg>{};
            if (controller.mTagArg.mArgType == CustomTagArg::Type_Int) {
                result.push_back(smgpc::resource::BmgFormatArg::number(controller.mTagArg.mIntArg));
            } else if (controller.mTagArg.mArgType == CustomTagArg::Type_Char) {
                result.push_back(smgpc::resource::BmgFormatArg::string(to_utf16(controller.mTagArg.mCharArg)));
            }
            return result;
        }

        [[nodiscard]] TalkPresentation make_presentation(TalkMessageCtrl& controller) const {
            const auto& controller_state = state(controller);
            const auto message_index = controller.getMessageID();
            const auto* message_id = messages().message_id(message_index);
            if (message_id == nullptr) {
                throw std::logic_error("Current talk message is outside MessageId.tbl.");
            }
            const auto* raw_text = messages().message_raw_utf16(*message_id);
            const auto* display_text = messages().message_utf16(*message_id);
            const auto* info = messages().message_info(*message_id);
            if (raw_text == nullptr || display_text == nullptr || info == nullptr) {
                throw std::logic_error("Current talk presentation data is unavailable.");
            }
            auto formatted = *display_text;
            const auto args = format_args(controller);
            if (!args.empty()) {
                formatted = messages().format_message_utf16(*message_id, args);
            }
            return TalkPresentation{
                .controller = &controller,
                .flow_key = controller_state.flow_key,
                .message_id = *message_id,
                .message_index = message_index,
                .node_index = node_index(controller.mNodeCtrl->mCurrentNode),
                .raw_text = *raw_text,
                .display_text = std::move(formatted),
                .info = *info,
                .time_keep_paused = false,
            };
        }

        void trace(const TalkMessageCtrl& controller, std::string_view event) const {
#ifndef NDEBUG
            auto detail = std::string("host=") +
                          (controller.mHostActor != nullptr && controller.mHostActor->getName() != nullptr
                               ? controller.mHostActor->getName()
                               : "") +
                          ";flow=" + std::string(state(controller).flow_key) +
                          ";message_id=" + std::to_string(controller.getMessageID());
            runtime_context().emit_semantic_trace_event("talk", event, detail);
#else
            static_cast<void>(controller);
            static_cast<void>(event);
#endif
        }

        [[nodiscard]] bool request(TalkMessageCtrl& controller, bool force) {
            auto& controller_state = state(controller);
            controller_state.start_latch = false;
            controller_state.end_latch = false;

            if (controller.mIsOnRootNodeAuto) {
                controller.rootNodePre(false);
            }
            if (!force && !controller.isNearPlayer(controller.mTalkDistance)) {
                return false;
            }
            if (active != nullptr && active != &controller) {
                return false;
            }

            controller_state.last_request_tick = tick;
            if (controller._18 == 0U) {
                controller._18 = 1U;
            }

            if (active == &controller) {
                return true;
            }
            if (force) {
                selected = &controller;
                pending = &controller;
                return true;
            }
            if (selected == &controller) {
                pending = &controller;
                return true;
            }
            if (pending == nullptr || controller.isNearPlayer(pending)) {
                pending = &controller;
            }
            return false;
        }

        [[nodiscard]] bool start(TalkMessageCtrl& controller, bool force, bool use_demo,
                                 bool) {
            auto& controller_state = state(controller);
            if (active == &controller) {
                return true;
            }
            if (!force && selected != &controller) {
                return false;
            }
            if (force) {
                selected = &controller;
            }

            controller.rootNodePre(true);
            if (controller.mNodeCtrl->mCurrentNode == nullptr &&
                !controller_state.direct_message_index.has_value()) {
                throw std::logic_error("Talk start reached no message node.");
            }
            if (controller.mNodeCtrl->isCurrentNodeEvent()) {
                if (!controller.rootNodeEve()) {
                    return false;
                }
                controller.rootNodePre(true);
            }
            if (controller.mNodeCtrl->mCurrentNode != nullptr &&
                controller.mNodeCtrl->mCurrentNode->mNodeType != 1U) {
                throw std::logic_error("Talk start did not resolve to a message node.");
            }

            const auto is_short = controller.mNodeCtrl->mMessageInfo.isShortTalk();
            auto* demo = active_demo_scene_runtime();
            if (use_demo && !is_short && (demo == nullptr || !demo->is_time_keep_active())) {
                throw std::logic_error(
                    "Normal programmable talk-demo ownership is unavailable; an event talk may only use the active time-keep demo runtime.");
            }

            active = &controller;
            active_force = force;
            presentation = make_presentation(controller);
            controller_state.start_latch = true;
            controller_state.end_latch = false;
            controller._18 = 3U;
            const auto& wpad = runtime_context().wpad();
            input_released_after_open = !wpad.is_button_held(WPAD_CHAN0, WPAD_BUTTON_A);

            if (use_demo && !is_short && demo != nullptr && demo->is_time_keep_active()) {
                demo->pause_time_keep(controller.mHostActor);
                active_time_keep_pause = true;
                presentation->time_keep_paused = true;
            }
            trace(controller, "start");
            return true;
        }

        void finish_active(bool apply_root_progression) {
            if (active == nullptr) {
                return;
            }
            auto* controller = active;
            auto& controller_state = state(*controller);
            const auto presented_message = presentation.has_value()
                                               ? std::optional<std::uint32_t>(presentation->message_index)
                                               : std::nullopt;
            resume_time_keep_if_needed();
            controller->readMessage();
            if (apply_root_progression && controller->mIsOnRootNodeAuto &&
                (!presented_message.has_value() || controller->getMessageID() == *presented_message)) {
                controller->rootNodePst();
            }
            controller->_18 = 4U;
            controller_state.end_latch = true;
            controller_state.start_latch = false;
            trace(*controller, "end");
            presentation.reset();
            active = nullptr;
            selected = nullptr;
            active_force = false;
        }

        void move() {
            ++tick;
            if (active != nullptr) {
                auto& active_state = state(*active);
                if (!presentation.has_value()) {
                    throw std::logic_error(
                        "An active talk controller has no frozen presentation.");
                }
                if (presentation->info.talk_type == 1U) {
                    if (active_state.last_request_tick + 1U < tick) {
                        finish_active(true);
                    }
                } else {
                    const auto& wpad = runtime_context().wpad();
                    const auto held = wpad.is_button_held(WPAD_CHAN0, WPAD_BUTTON_A);
                    if (!held) {
                        input_released_after_open = true;
                    }
                    if (input_released_after_open &&
                        wpad.is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A)) {
                        finish_active(true);
                    }
                }
            }

            if (active == nullptr) {
                selected = pending;
            }
            pending = nullptr;
        }

        void shutdown() {
            resume_time_keep_if_needed();
            active = nullptr;
            pending = nullptr;
            selected = nullptr;
            presentation.reset();

            for (auto& [actor, controller] : owned_controllers) {
                if (auto* npc = dynamic_cast<NPCActor*>(const_cast<LiveActor*>(actor));
                    npc != nullptr && npc->mMsgCtrl == controller.get()) {
                    npc->mMsgCtrl = nullptr;
                }
            }
            owned_controllers.clear();
            for (auto& [controller, controller_state] : controllers) {
                controller->mNodeCtrl = nullptr;
                controller->mCameraInfo = nullptr;
                controller->mBranchFunc = nullptr;
                controller->mEventFunc = nullptr;
                controller->mAnimeFunc = nullptr;
                controller->mKillFunc = nullptr;
            }
            controller_by_node.clear();
            controllers.clear();
        }
    };

    TalkRuntime::TalkRuntime()
        : NameObj("会話統括"), _impl(std::make_unique<Impl>(*this)) {
    }

    TalkRuntime::~TalkRuntime() {
        _impl->shutdown();
        if (_impl->bound) {
            if (sCurrentTalkRuntime != this) {
                std::terminate();
            }
            sCurrentTalkRuntime = nullptr;
            _impl->bound = false;
        }
    }

    void TalkRuntime::init(const JMapInfoIter&) {
        if (_impl->bound) {
            throw std::logic_error("TalkRuntime was initialized twice.");
        }
        if (sCurrentTalkRuntime != nullptr) {
            throw std::logic_error("a TalkRuntime is already bound to the active scene");
        }
        static_cast<void>(_impl->runtime_context());
        sCurrentTalkRuntime = this;
        _impl->bound = true;
        MR::connectToScene(this, MR::MovementType_TalkDirector, -1, -1, -1);
    }

    void TalkRuntime::movement() {
        _impl->move();
    }

    const std::optional<TalkPresentation>& TalkRuntime::active_presentation() const {
        return _impl->presentation;
    }

    std::optional<std::uint32_t> TalkRuntime::current_node_index(
        const TalkMessageCtrl& controller) const {
        return _impl->node_index(_impl->state(controller).node_ctrl->mCurrentNode);
    }

    std::string_view TalkRuntime::flow_key(const TalkMessageCtrl& controller) const {
        return _impl->state(controller).flow_key;
    }

    TalkMessageCtrl* TalkRuntime::adopt_owned_controller(
        LiveActor* actor, std::unique_ptr<TalkMessageCtrl> controller) {
        if (actor == nullptr || controller == nullptr) {
            throw std::logic_error("TalkRuntime cannot adopt a null actor or controller.");
        }
        release_owned_controller(actor);
        auto* result = controller.get();
        _impl->owned_controllers.emplace(actor, std::move(controller));
        return result;
    }

    TalkMessageCtrl* TalkRuntime::owned_controller(const LiveActor* actor) const {
        const auto found = _impl->owned_controllers.find(actor);
        return found != _impl->owned_controllers.end() ? found->second.get() : nullptr;
    }

    void TalkRuntime::release_owned_controller(const LiveActor* actor) {
        const auto found = _impl->owned_controllers.find(actor);
        if (found == _impl->owned_controllers.end()) {
            return;
        }
        auto controller = std::move(found->second);
        _impl->owned_controllers.erase(found);
        if (auto* npc = dynamic_cast<NPCActor*>(const_cast<LiveActor*>(actor));
            npc != nullptr && npc->mMsgCtrl == controller.get()) {
            npc->mMsgCtrl = nullptr;
        }
        controller.reset();
    }

    bool TalkRuntime::has_owned_controller(const LiveActor* actor) const {
        return _impl->owned_controllers.contains(actor);
    }

    bool TalkRuntime::consume_end(const TalkMessageCtrl& controller) {
        auto& controller_state = _impl->state(const_cast<TalkMessageCtrl&>(controller));
        if (!controller_state.end_latch) {
            return false;
        }
        controller_state.end_latch = false;
        const_cast<TalkMessageCtrl&>(controller)._18 = 0U;
        return true;
    }

    TalkRuntime* current_talk_runtime() noexcept {
        return sCurrentTalkRuntime;
    }

    TalkRuntime& require_talk_runtime(std::string_view operation) {
        auto* runtime = current_talk_runtime();
        if (runtime == nullptr) {
            throw std::logic_error(std::string(operation) +
                                   " requires the active scene-owned TalkRuntime.");
        }
        return *runtime;
    }

}  // namespace smgpc::compat

TalkMessageInfo::TalkMessageInfo()
    : _0(nullptr), mCameraSetID(0U), _6(0), mCameraType(2U), mTalkType(0U),
      mBalloonType(0U), _A(-1), _B(-1) {
}

bool TalkMessageInfo::isNormalTalk() const { return mTalkType == 0U; }
bool TalkMessageInfo::isShortTalk() const { return mTalkType == 1U; }
bool TalkMessageInfo::isEventTalk() const { return mTalkType == 2U; }
bool TalkMessageInfo::isComposeTalk() const { return mTalkType == 3U; }
bool TalkMessageInfo::isFlowTalk() const { return mTalkType == 4U; }
bool TalkMessageInfo::isNullTalk() const { return mTalkType == 5U; }
bool TalkMessageInfo::isBalloonCall() const { return mBalloonType == 2U; }
bool TalkMessageInfo::isBalloonFix() const { return mBalloonType == 3U; }
bool TalkMessageInfo::isBalloonSign() const { return mBalloonType == 4U; }
bool TalkMessageInfo::isBalloonInfo() const { return mBalloonType == 5U; }
bool TalkMessageInfo::isBalloonIcon() const { return mBalloonType == 6U; }
bool TalkMessageInfo::isCameraNormal() const { return mCameraType == 0U; }
bool TalkMessageInfo::isCameraEvent() const { return mCameraType == 1U; }

void TalkMessageHistory::entry(u16 message_id) {
    if (mCount < static_cast<s32>(std::size(mHistory))) {
        mHistory[mCount++] = message_id;
    }
}

bool TalkMessageHistory::search(u16 message_id) const {
    for (auto index = s32{}; index < mCount; ++index) {
        if (mHistory[index] == message_id) {
            return true;
        }
    }
    return false;
}

TalkNodeCtrl::TalkNodeCtrl()
    : _0(nullptr), mCurrentNodeIdx(-1), mMessageInfo(), mHistory{}, _38(nullptr),
      mCurrentNode(nullptr), mFlowNode(nullptr), mNodeData(-1) {
    mHistory.mCount = 0;
}

void TalkNodeCtrl::resetFlowNode() {
    if (mCurrentNode != _38) {
        mCurrentNode = _38;
        mFlowNode = _38;
        updateMessage();
    }
}

void TalkNodeCtrl::resetTempFlowNode() {
    if (mCurrentNode != mFlowNode) {
        mCurrentNode = mFlowNode;
        updateMessage();
    }
}

void TalkNodeCtrl::recordTempFlowNode() {
    mFlowNode = mCurrentNode;
}

void TalkNodeCtrl::forwardFlowNode() {
    auto& runtime = smgpc::compat::require_talk_runtime("Talk flow traversal");
    if (mCurrentNode == nullptr) {
        return;
    }
    if (mCurrentNode->mNodeType == 1U) {
        if (mCurrentNode->mNextIdx == 0xffffU) {
            mCurrentNode = nullptr;
        } else {
            auto* target = runtime._impl->node(mCurrentNode->mNextIdx);
            if (target == nullptr) {
                throw std::logic_error("Talk message node points outside the retained FLW node table.");
            }
            mCurrentNode = target;
        }
    } else if (mCurrentNode->mNodeType == 3U) {
        auto* target = runtime._impl->branch_node(mCurrentNode->mIndex);
        if (target == nullptr) {
            throw std::logic_error("Talk event node points outside the retained FLW branch table.");
        }
        mCurrentNode = target;
    } else if (mCurrentNode->mNodeType != 2U) {
        throw std::logic_error("Talk flow contains an unknown node type.");
    }
    updateMessage();
}

bool TalkNodeCtrl::isExistNextNode() const {
    return getNextNode() != nullptr;
}

bool TalkNodeCtrl::isNextNodeMessage() const {
    const auto* next = getNextNode();
    return next != nullptr && next->mNodeType == 1U;
}

bool TalkNodeCtrl::isCurrentNodeEvent() const {
    return getCurrentNodeEvent() != nullptr;
}

TalkNode* TalkNodeCtrl::getNextNode() const {
    auto& runtime = smgpc::compat::require_talk_runtime("Talk next-node lookup");
    return runtime._impl->next_node(*this);
}

TalkNode* TalkNodeCtrl::getNextNodeBranch() const {
    auto* next = getNextNode();
    return next != nullptr && next->mNodeType == 2U ? next : nullptr;
}

TalkNode* TalkNodeCtrl::getCurrentNodeBranch() const {
    return mCurrentNode != nullptr && mCurrentNode->mNodeType == 2U ? mCurrentNode : nullptr;
}

TalkNode* TalkNodeCtrl::getCurrentNodeMessage() const {
    return mCurrentNode != nullptr && mCurrentNode->mNodeType == 1U ? mCurrentNode : nullptr;
}

TalkNode* TalkNodeCtrl::getCurrentNodeEvent() const {
    return mCurrentNode != nullptr && mCurrentNode->mNodeType == 3U ? mCurrentNode : nullptr;
}

TalkNode* TalkNodeCtrl::getNextNodeEvent() const {
    auto* next = getNextNode();
    return next != nullptr && next->mNodeType == 3U ? next : nullptr;
}

void TalkNodeCtrl::updateMessage() {
    auto& runtime = smgpc::compat::require_talk_runtime("Talk message update");
    runtime._impl->update_message(*this);
}

void TalkNodeCtrl::readMessage() {
    if (mMessageInfo.isEventTalk() && !mHistory.search(static_cast<u16>(mCurrentNodeIdx))) {
        mHistory.entry(static_cast<u16>(mCurrentNodeIdx));
        mMessageInfo.mTalkType = 0U;
    }
    if (mMessageInfo._B != -1) {
        MR::onMessageAlreadyRead(mMessageInfo._B);
    }
}

void TalkNodeCtrl::forwardCurrentBranchNode(bool left) {
    if (mCurrentNode == nullptr || mCurrentNode->mNodeType != 2U) {
        throw std::logic_error("Talk branch traversal requires a current branch node.");
    }
    auto& runtime = smgpc::compat::require_talk_runtime("Talk branch traversal");
    mCurrentNode = runtime._impl->branch_node(
        static_cast<std::uint32_t>(mCurrentNode->mNextGroup) + (left ? 0U : 1U));
    if (mCurrentNode == nullptr) {
        throw std::logic_error("Talk branch target is absent from the retained FLW branch table.");
    }
    updateMessage();
}

void TalkNodeCtrl::createFlowNode(TalkMessageCtrl* controller, const JMapInfoIter& iter,
                                  const char* name, ActorCameraInfo** camera_info) {
    auto message_id = s32{-1};
    if (!MR::getJMapInfoMessageID(iter, &message_id) || message_id < 0) {
        throw std::logic_error("Placement talk creation requires a non-negative MessageId.");
    }
    const auto* zone_name = MR::getCurrentPlacementZoneName();
    if (zone_name == nullptr || name == nullptr) {
        throw std::logic_error("Placement talk creation requires a zone name and actor message name.");
    }
    char flow_key[0x100]{};
    std::snprintf(flow_key, sizeof(flow_key), "%s_%s%03d", zone_name, name, message_id);
    createFlowNodeDirect(controller, iter, flow_key, camera_info);
}

void TalkNodeCtrl::createFlowNodeDirect(TalkMessageCtrl* controller, const JMapInfoIter& iter,
                                        const char* flow_key, ActorCameraInfo** camera_info) {
    if (controller == nullptr || flow_key == nullptr) {
        throw std::logic_error("Direct talk creation requires a controller and flow key.");
    }
    auto& runtime = smgpc::compat::require_talk_runtime("Direct talk creation");
    runtime._impl->create_message_direct(*controller, iter, flow_key, camera_info);
}

const wchar_t* TalkNodeCtrl::getSubMessage() const {
    return nullptr;
}

void TalkNodeCtrl::initNodeRecursive(TalkMessageCtrl*, const JMapInfoIter&, ActorCameraInfo*,
                                     RecursiveHelper*) {
    // Camera rows remain represented by ActorCameraInfo. Recursive retail
    // camera registration is intentionally deferred to the camera provider;
    // flow traversal itself is complete and does not special-case a scene.
}

inline bool RecursiveHelper::hasNode(const TalkNode* node) const {
    for (auto index = s32{}; index < mIndex; ++index) {
        if (mStack[index] == node) {
            return true;
        }
    }
    return false;
}

TalkMessageCtrl::TalkMessageCtrl(LiveActor* host, const TVec3f& offset, MtxPtr matrix)
    : NameObj("会話制御"), mHostActor(host), mNodeCtrl(nullptr), mZoneID(-1), _18(0U),
      _1C(0.0F, 0.0F, 0.0F), mMtx(matrix), _2C(offset), mTalkDistance(240.0F), _3C(0U),
      mAlreadyDoneFlags(0U), mIsOnRootNodeAuto(false), mIsOnReadNodeAuto(true),
      mIsStartOnlyFront(false), mCameraInfo(nullptr), mBranchFunc(nullptr), mEventFunc(nullptr),
      mAnimeFunc(nullptr), mKillFunc(nullptr),
      mTagArg(0, CustomTagArg::Type_Uninitialized) {
    MR::createSceneObj(SceneObj_TalkDirector);
    TalkFunction::registerTalkSystem(this);
    if (MR::isExistSceneObj(SceneObj_PlacementStateChecker)) {
        mZoneID = MR::getCurrentPlacementZoneId();
    }
}

TalkMessageCtrl::~TalkMessageCtrl() {
    if (auto* runtime = smgpc::compat::current_talk_runtime(); runtime != nullptr) {
        runtime->_impl->unregister_controller(*this);
    }
}

void TalkMessageCtrl::createMessage(const JMapInfoIter& iter, const char* name) {
    mNodeCtrl->createFlowNode(this, iter, name, &mCameraInfo);
    if (mNodeCtrl->mCurrentNode != nullptr) {
        mAlreadyDoneFlags = MR::setupAlreadyDoneFlag(mNodeCtrl->_0, iter, &_3C);
    } else {
        _3C = 1U;
    }
}

void TalkMessageCtrl::createMessageDirect(const JMapInfoIter& iter, const char* name) {
    mNodeCtrl->createFlowNodeDirect(this, iter, name, &mCameraInfo);
    _3C = 1U;
}

u32 TalkMessageCtrl::getMessageID() const {
    if (mNodeCtrl == nullptr) {
        throw std::logic_error("TalkMessageCtrl has no active TalkNodeCtrl.");
    }
    if (const auto* message = mNodeCtrl->getCurrentNodeMessage(); message != nullptr) {
        return message->mIndex;
    }
    if (mNodeCtrl->mCurrentNodeIdx < 0) {
        throw std::logic_error("TalkMessageCtrl has no current message.");
    }
    return static_cast<u32>(mNodeCtrl->mCurrentNodeIdx);
}

bool TalkMessageCtrl::requestTalk() {
    return TalkFunction::requestTalkSystem(this, false);
}

bool TalkMessageCtrl::requestTalkForce() {
    return TalkFunction::requestTalkSystem(this, true);
}

void TalkMessageCtrl::startTalk() {
    TalkFunction::startTalkSystem(this, false, true, true);
}

void TalkMessageCtrl::startTalkForce() {
    TalkFunction::startTalkSystem(this, true, true, true);
}

void TalkMessageCtrl::startTalkForcePuppetable() {
    TalkFunction::startTalkSystem(this, true, true, false);
}

void TalkMessageCtrl::startTalkForceWithoutDemo() {
    TalkFunction::startTalkSystem(this, true, false, true);
}

void TalkMessageCtrl::startTalkForceWithoutDemoPuppetable() {
    TalkFunction::startTalkSystem(this, true, false, false);
}

void TalkMessageCtrl::endTalk() {
    static_cast<void>(smgpc::compat::require_talk_runtime("Talk end query")
                          ._impl->state(*this).end_latch);
}

bool TalkMessageCtrl::isNearPlayer(const TalkMessageCtrl* other) {
    if (other == nullptr) {
        return true;
    }
    const auto* player = MR::getPlayerPos();
    if (player == nullptr || mHostActor == nullptr || other->mHostActor == nullptr) {
        return false;
    }
    return mHostActor->mPosition.squared(*player) < other->mHostActor->mPosition.squared(*player);
}

bool TalkMessageCtrl::isNearPlayer(f32 distance) const {
    if (mNodeCtrl == nullptr || mNodeCtrl->mMessageInfo.isNullTalk() || mHostActor == nullptr) {
        return false;
    }
    if (mTalkDistance < 0.0F) {
        return true;
    }
    if (distance <= 0.0F) {
        distance = mTalkDistance;
        if (mNodeCtrl->mMessageInfo.isComposeTalk()) {
            distance *= 2.0F;
        }
    }
    return MR::isNearPlayer(mHostActor, distance) || inMessageArea();
}

bool TalkMessageCtrl::inMessageArea() const {
    if (mNodeCtrl == nullptr || mNodeCtrl->mMessageInfo._A < 0) {
        return false;
    }
    const auto* player_position = MR::getPlayerCenterPos();
    if (player_position == nullptr) {
        return false;
    }
    const auto* area = static_cast<MessageArea*>(MR::getAreaObj("MessageArea", *player_position));
    return area != nullptr && mNodeCtrl->mMessageInfo._A == area->mObjArg0 &&
           mZoneID == area->mZoneID;
}

void TalkMessageCtrl::updateBalloonPos() {
}

void TalkMessageCtrl::rootNodePre(bool stop_at_non_branch) {
    mNodeCtrl->resetTempFlowNode();
    while (true) {
        auto* branch = mNodeCtrl->getCurrentNodeBranch();
        if (branch == nullptr) {
            if (stop_at_non_branch || !isCurrentNodeContinue()) {
                return;
            }
            mNodeCtrl->forwardFlowNode();
            continue;
        }

        auto condition = false;
        switch (branch->mIndex) {
        case 0:
            return;
        case 1:
            condition = mBranchFunc == nullptr || (*mBranchFunc)(branch->mNextIdx);
            break;
        case 2:
            condition = MR::isNearPlayerAnyTime(mHostActor, mTalkDistance);
            break;
        case 3:
            condition = MR::isOnSwitchA(mHostActor);
            break;
        case 4:
            condition = MR::isOnSwitchB(mHostActor);
            break;
        case 5:
            condition = MR::isPlayerElementModeNormal();
            break;
        case 6:
            condition = MR::isPlayerElementModeBee();
            break;
        case 7:
            condition = MR::isPlayerElementModeTeresa();
            break;
        case 8:
            throw std::logic_error("Talk branch type 8 requires the PowerStar-appeared stage-state provider.");
        case 9:
            condition = _3C != 0U;
            break;
        case 10:
            condition = MR::isPlayerLuigi();
            break;
        case 11: {
            const auto* demo = smgpc::compat::active_demo_scene_runtime();
            condition = demo != nullptr && demo->is_time_keep_active();
            break;
        }
        case 12:
            condition = MR::isOnMessageAlreadyRead(static_cast<s8>(branch->mNextIdx));
            break;
        case 13:
            condition = MR::isMsgLedPattern();
            break;
        case 14:
            condition = TalkFunction::getBranchAstroGalaxyResult(branch->mNextIdx);
            break;
        default:
            throw std::logic_error("Talk flow contains an unsupported branch type.");
        }
        mNodeCtrl->forwardCurrentBranchNode(condition);
    }
}

void TalkMessageCtrl::rootNodePst() {
    if (!mNodeCtrl->isExistNextNode()) {
        mNodeCtrl->resetFlowNode();
    } else {
        mNodeCtrl->forwardFlowNode();
    }
    mNodeCtrl->recordTempFlowNode();
}

bool TalkMessageCtrl::isCurrentNodeContinue() const {
    return event_node_continues(mNodeCtrl->getCurrentNodeEvent());
}

bool TalkMessageCtrl::rootNodeEve() {
    auto* event = mNodeCtrl->getCurrentNodeEvent();
    if (event == nullptr) {
        return true;
    }
    const auto callback_arg = (static_cast<u32>(event->mNextIdx) << 16U) |
                              static_cast<u32>(event->mNextGroup);
    if (event->mGroupID <= 1U) {
        if (mEventFunc != nullptr && !(*mEventFunc)(callback_arg)) {
            return false;
        }
    } else if (event->mGroupID == 4U) {
        if (mAnimeFunc != nullptr && !(*mAnimeFunc)(callback_arg)) {
            return false;
        }
    } else if (event->mGroupID == 3U) {
        rootNodePst();
        return true;
    } else if (event->mGroupID == 5U) {
        MR::onSwitchA(mHostActor);
    } else if (event->mGroupID == 6U) {
        MR::onSwitchB(mHostActor);
    } else if (event->mGroupID == 7U) {
        if (mKillFunc == nullptr) {
            throw std::logic_error("Talk kill event has no registered callback.");
        }
        if (!(*mKillFunc)(callback_arg)) {
            return false;
        }
    }

    const auto had_next = mNodeCtrl->isExistNextNode();
    rootNodePst();
    return !(had_next && mNodeCtrl->getCurrentNodeEvent() != nullptr);
}

void TalkMessageCtrl::rootNodeSel(bool left) {
    mNodeCtrl->forwardFlowNode();
    mNodeCtrl->forwardCurrentBranchNode(left);
    mNodeCtrl->recordTempFlowNode();
}

void TalkMessageCtrl::registerBranchFunc(const TalkMessageFuncBase& function) {
    auto& allocation = smgpc::compat::require_talk_runtime("Talk branch callback registration")
                           ._impl->state(*this).branch_func;
    allocation.reset(function.clone());
    mBranchFunc = allocation.value;
}

void TalkMessageCtrl::registerEventFunc(const TalkMessageFuncBase& function) {
    auto& allocation = smgpc::compat::require_talk_runtime("Talk event callback registration")
                           ._impl->state(*this).event_func;
    allocation.reset(function.clone());
    mEventFunc = allocation.value;
}

void TalkMessageCtrl::registerAnimeFunc(const TalkMessageFuncBase& function) {
    auto& allocation = smgpc::compat::require_talk_runtime("Talk animation callback registration")
                           ._impl->state(*this).anime_func;
    allocation.reset(function.clone());
    mAnimeFunc = allocation.value;
}

void TalkMessageCtrl::registerKillFunc(const TalkMessageFuncBase& function) {
    auto& allocation = smgpc::compat::require_talk_runtime("Talk kill callback registration")
                           ._impl->state(*this).kill_func;
    allocation.reset(function.clone());
    mKillFunc = allocation.value;
}

void TalkMessageCtrl::readMessage() {
    if (mIsOnReadNodeAuto) {
        mNodeCtrl->readMessage();
    }
    if (_3C == 0U) {
        MR::updateAlreadyDoneFlag(mAlreadyDoneFlags, 1U);
    }
}

bool TalkMessageCtrl::isSelectYesNo() const {
    const auto* branch = mNodeCtrl->getNextNodeBranch();
    return branch != nullptr && branch->mIndex == 0U && branch->mNextIdx != 14U &&
           branch->mNextIdx != 16U;
}

void TalkMessageCtrl::startCamera(s32) {
    if (mNodeCtrl->mMessageInfo.isCameraNormal() || mNodeCtrl->mMessageInfo.isCameraEvent()) {
        throw std::logic_error("Talk camera dispatch requires the generalized talk-camera provider.");
    }
}

const char* TalkMessageCtrl::getBranchID() const {
    const auto* branch = mNodeCtrl->getNextNodeBranch();
    if (branch == nullptr || branch->mIndex != 0U) {
        return nullptr;
    }
    static constexpr const char* ids[] = {
        "PenguinRace", "SwimmingSchool", "PenguinRace", "BombTimeAttackLv1",
        "PhantomTeresaRacer", "BombTimeAttackLv2", "TrialSurfingCoach",
        "TrialSurfingHowTo", "DeathPromenadeTeresaRacer", "RosettaFinalBattle",
        "CometTico", "TransformTico", "ChallengeSurfingCoach", "TicoShopExchange",
        "TicoShopWhich", "KinopioPurple", "CometTicoTell", "TrialTamakoroHowTo",
        "KnockOnTheDoor", "LedPattern",
    };
    return branch->mNextIdx < std::size(ids) ? ids[branch->mNextIdx] : nullptr;
}

bool TalkFunction::isShortTalk(const TalkMessageCtrl* controller) {
    return controller != nullptr && controller->mNodeCtrl != nullptr &&
           controller->mNodeCtrl->mMessageInfo.isShortTalk();
}

bool TalkFunction::isComposeTalk(const TalkMessageCtrl* controller) {
    return controller != nullptr && controller->mNodeCtrl != nullptr &&
           controller->mNodeCtrl->mMessageInfo.isComposeTalk();
}

bool TalkFunction::isSelectTalk(const TalkMessageCtrl* controller) {
    return controller != nullptr && controller->mNodeCtrl != nullptr &&
           controller->mNodeCtrl->mNodeData == 0;
}

bool TalkFunction::isEventNode(const TalkMessageCtrl* controller) {
    return controller != nullptr && controller->mNodeCtrl != nullptr &&
           controller->mNodeCtrl->isCurrentNodeEvent();
}

bool TalkFunction::requestTalkSystem(TalkMessageCtrl* controller, bool force) {
    if (controller == nullptr) {
        return false;
    }
    return smgpc::compat::require_talk_runtime("Talk request")
        ._impl->request(*controller, force);
}

void TalkFunction::startTalkSystem(TalkMessageCtrl* controller, bool force, bool demo,
                                   bool player_not_puppetable) {
    if (controller == nullptr) {
        return;
    }
    static_cast<void>(smgpc::compat::require_talk_runtime("Talk start")
                          ._impl->start(*controller, force, demo, player_not_puppetable));
}

void TalkFunction::endTalkSystem(TalkMessageCtrl* controller) {
    if (controller != nullptr) {
        controller->endTalk();
    }
}

bool TalkFunction::isTalkSystemStart(const TalkMessageCtrl* controller) {
    return controller != nullptr &&
           smgpc::compat::require_talk_runtime("Talk start-state query")
               ._impl->state(*controller).start_latch;
}

bool TalkFunction::isTalkSystemEnd(const TalkMessageCtrl* controller) {
    return controller != nullptr &&
           smgpc::compat::require_talk_runtime("Talk end-state query")
               ._impl->state(*controller).end_latch;
}

bool TalkFunction::getBranchAstroGalaxyResult(u16) {
    throw std::logic_error("AstroGalaxy talk branches require the observatory progression provider.");
}

void TalkFunction::registerTalkSystem(TalkMessageCtrl* controller) {
    if (controller == nullptr) {
        throw std::logic_error("TalkRuntime cannot register a null TalkMessageCtrl.");
    }
    smgpc::compat::require_talk_runtime("TalkMessageCtrl construction")
        ._impl->register_controller(*controller);
}

TalkMessageInfo* TalkFunction::getMessageInfo(const TalkMessageCtrl* controller) {
    return controller != nullptr && controller->mNodeCtrl != nullptr
               ? &controller->mNodeCtrl->mMessageInfo
               : nullptr;
}

const wchar_t* TalkFunction::getSubMessage(const TalkMessageCtrl* controller) {
    return controller != nullptr && controller->mNodeCtrl != nullptr
               ? controller->mNodeCtrl->getSubMessage()
               : nullptr;
}

const wchar_t* TalkFunction::getMessage(const TalkMessageCtrl* controller) {
    const auto* info = getMessageInfo(controller);
    return info != nullptr ? reinterpret_cast<const wchar_t*>(info->_0) : nullptr;
}

void TalkFunction::onTalkStateEntry(TalkMessageCtrl* controller) {
    if (controller != nullptr) controller->_18 = 1U;
}
void TalkFunction::onTalkStateNone(TalkMessageCtrl* controller) {
    if (controller != nullptr) controller->_18 = 0U;
}
void TalkFunction::onTalkStateEnableStart(TalkMessageCtrl* controller) {
    if (controller != nullptr) controller->_18 = 2U;
}
void TalkFunction::onTalkStateTalking(TalkMessageCtrl* controller) {
    if (controller != nullptr) controller->_18 = 3U;
}
void TalkFunction::onTalkStateEnableEnd(TalkMessageCtrl* controller) {
    if (controller != nullptr) controller->_18 = 4U;
}
