#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "resource/BmgMessageArchive.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class JMapInfoIter;
class LiveActor;
class TalkFunction;
class TalkMessageCtrl;
class TalkMessageFuncBase;
class TalkNodeCtrl;

namespace smgpc::compat {

    struct TalkPresentation {
        const TalkMessageCtrl* controller = nullptr;
        std::string flow_key;
        std::string message_id;
        std::uint32_t message_index = 0U;
        std::optional<std::uint32_t> node_index;
        std::u16string raw_text;
        std::u16string display_text;
        smgpc::resource::BmgMessageInfo info{};
        bool time_keep_paused = false;
    };

    // Scene-owned compatibility counterpart of TalkDirector. The only global
    // surface is a non-owning pointer installed by init() and removed by the
    // destructor; message graphs, controllers, presentation, and input state
    // all remain owned by this scene object.
    class TalkRuntime final : public NameObj {
    public:
        TalkRuntime();
        ~TalkRuntime() override;

        TalkRuntime(const TalkRuntime&) = delete;
        TalkRuntime& operator=(const TalkRuntime&) = delete;

        void init(const JMapInfoIter&) override;
        void movement() override;

        [[nodiscard]] const std::optional<TalkPresentation>& active_presentation() const;
        [[nodiscard]] std::optional<std::uint32_t> current_node_index(const TalkMessageCtrl&) const;
        [[nodiscard]] std::string_view flow_key(const TalkMessageCtrl&) const;

        [[nodiscard]] TalkMessageCtrl* adopt_owned_controller(
            LiveActor*, std::unique_ptr<TalkMessageCtrl>);
        // NPCActor::mMsgCtrl is the preferred retail placement controller
        // while it names an owned identity; otherwise the first controller is
        // the stable fallback. Additional direct controllers (for example
        // Tico's Common_Tico000 reaction flow) never replace either identity.
        [[nodiscard]] TalkMessageCtrl* owned_controller(const LiveActor*) const;
        [[nodiscard]] std::size_t owned_controller_count(const LiveActor*) const;
        // Actor teardown is an all-or-nothing boundary for every controller
        // retained under the actor identity.
        void release_owned_controller(const LiveActor*);
        [[nodiscard]] bool has_owned_controller(const LiveActor*) const;

        // The AtEnd helpers consume the one completed-talk edge. Plain
        // isTalkEnd remains a non-consuming state query.
        [[nodiscard]] bool consume_end(const TalkMessageCtrl&);

    private:
        friend class ::TalkFunction;
        friend class ::TalkMessageCtrl;
        friend class ::TalkNodeCtrl;

        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    [[nodiscard]] TalkRuntime* current_talk_runtime() noexcept;
    [[nodiscard]] TalkRuntime& require_talk_runtime(std::string_view operation);

}  // namespace smgpc::compat
