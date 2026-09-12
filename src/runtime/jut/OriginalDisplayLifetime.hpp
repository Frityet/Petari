#pragma once

#include "render/RendererService.hpp"

#include <memory>

class JUTDirectPrint;
class JUTVideo;
class MainLoopFramework;

namespace smgpc::compat {
    class JkrAllocationDomain;
    class JkrHeapRuntime;
}

namespace smgpc::runtime {
    // Owns the same original display factories used by GameSystemObjHolder
    // for native hosts that exercise scenes without constructing GameSystem.
    class OriginalDisplayLifetime final : public render::DisplayFrameSource {
    public:
        OriginalDisplayLifetime(render::AuroraWindow&, std::shared_ptr<compat::JkrHeapRuntime>,
                                const GXRenderModeObj&);
        ~OriginalDisplayLifetime();
        OriginalDisplayLifetime(const OriginalDisplayLifetime&) = delete;
        OriginalDisplayLifetime& operator=(const OriginalDisplayLifetime&) = delete;

        [[nodiscard]] GXRenderModeObj& render_mode() const;
        [[nodiscard]] MainLoopFramework& main_loop() const;
        void begin_render(const render::CopyClearState&) override;
        void end_render() override;
        void set_copy_clear(const render::CopyClearState&) override;

    private:
        void retire();
        render::AuroraWindow& _window;
        std::shared_ptr<compat::JkrAllocationDomain> _domain;
        GXRenderModeObj _mode;
        JUTDirectPrint* _direct = nullptr;
        JUTVideo* _video = nullptr;
        MainLoopFramework* _main_loop = nullptr;
        GXDrawDoneCallback _previous_draw_callback = nullptr;
        void* _buffers[3]{};
        bool _attached = false;
        bool _rendering = false;
        bool _draw_callback_installed = false;
    };
}
