from pathlib import Path
p=Path('src/compat/TalkRuntime.cpp');s=p.read_text();s=s.replace('#include "Game/NPC/TalkNodeCtrl.hpp"','#include "Game/NPC/TalkNodeCtrl.hpp"\n#include "Game/System/MessageHolder.hpp"')
a=s.index('    [[nodiscard]] std::wstring to_wide(');b=s.index('    [[nodiscard]] std::u16string to_utf16(',a);s=s[:a]+s[b:]
s=s.replace('        struct ControllerState {\n            std::unique_ptr<TalkNodeCtrl> node_ctrl;', '''        struct NodeDeleter {
            void operator()(TalkNodeCtrl* node) const noexcept {
                if (!node) return;
                delete[] node->_0;
                delete node;
            }
        };

        struct ControllerState {
            std::unique_ptr<TalkNodeCtrl, NodeDeleter> node_ctrl;''')
s=s.replace('            std::string flow_key;\n            std::wstring current_message;\n            std::optional<std::uint32_t> direct_message_index;\n','')
s=s.replace('        bool nodes_loaded = false;\n','').replace('        std::vector<TalkNode> nodes;\n','').replace('        std::unordered_map<const TalkNodeCtrl*, TalkMessageCtrl*> controller_by_node;\n','')
a=s.index('        [[nodiscard]] TalkMessageCtrl& controller(TalkNodeCtrl&');b=s.index('        void register_controller(',a)
s=s[:a]+'''        [[nodiscard]] std::optional<std::uint32_t> node_index(const TalkNode* wanted) const {
            if (!wanted) return std::nullopt;
            const auto* data = MessageSystem::getSceneMessageData();
            if (!data || !data->mFlowBlock) return std::nullopt;
            for (std::uint32_t i = 0; i < data->mFlowBlock->mNodeCount; ++i)
                if (data->getNode(i) == wanted) return i;
            return std::nullopt;
        }

        [[nodiscard]] std::uint32_t presentation_message_index(const TalkMessageCtrl& controller) const {
            const auto* node = controller.mNodeCtrl;
            if (node->mCurrentNode || node->mCurrentNodeIdx >= 0) return controller.getMessageID();
            // A direct non-flow message leaves the original node index at -1.
            // Resolve only native presentation metadata; never mutate Game state.
            const auto index = MessageSystem::getSceneMessageData()->findMessageIndex(node->_0);
            if (index < 0)
                aurora::throw_host_exception<std::logic_error>("Direct talk presentation has no authored message identifier.");
            return static_cast<std::uint32_t>(index);
        }

'''+s[b:]
s=s.replace('controller_state.node_ctrl = std::make_unique<TalkNodeCtrl>();','controller_state.node_ctrl.reset(new TalkNodeCtrl);')
s=s.replace('            controller_by_node.emplace(controller.mNodeCtrl, &controller);\n','').replace('            controller_by_node.erase(found->second.node_ctrl.get());\n','').replace('            controller_by_node.clear();\n','')
a=s.index('        void create_message_direct(');b=s.index('        [[nodiscard]] std::vector<smgpc::resource::BmgFormatArg>',a);s=s[:a]+s[b:]
s=s.replace('const auto message_index = controller.getMessageID();','const auto message_index = presentation_message_index(controller);')
s=s.replace('.flow_key = controller_state.flow_key,','.flow_key = controller.mNodeCtrl->_0,')
s=s.replace('std::string(state(controller).flow_key)','std::string(controller.mNodeCtrl->_0 ? controller.mNodeCtrl->_0 : "")')
s=s.replace('!controller_state.direct_message_index.has_value()', 'controller.mNodeCtrl->mMessageInfo._0 == nullptr')
s=s.replace('return _impl->state(controller).flow_key;','const auto* key = _impl->state(controller).node_ctrl->_0;\n        return key ? std::string_view(key) : std::string_view{};')
a=s.index('void TalkMessageHistory::entry(');b=s.index('TalkMessageCtrl::TalkMessageCtrl(',a);s=s[:a]+s[b:]
# Retain the original camera allocation, including exceptions during recursive registration.
for sig in ['void TalkMessageCtrl::createMessage(const JMapInfoIter& iter, const char* name) {','void TalkMessageCtrl::createMessageDirect(const JMapInfoIter& iter, const char* name) {']:
 s=s.replace(sig,sig+'''
    auto& state = smgpc::compat::require_talk_runtime("Original talk creation")._impl->state(*this);
    struct CameraLifetime {
        std::unique_ptr<ActorCameraInfo>& owner;
        ActorCameraInfo*& camera;
        ~CameraLifetime() { if (owner.get() != camera) owner.reset(camera); }
    } lifetime{state.camera_info, mCameraInfo};''')
p.write_text(s)
