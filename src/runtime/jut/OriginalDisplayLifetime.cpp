#include "runtime/jut/OriginalDisplayLifetime.hpp"

#include "Game/System/MainLoopFramework.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include "JSystem/JUtility/JUTDirectPrint.hpp"
#include "JSystem/JUtility/JUTVideo.hpp"
#include "JSystem/JUtility/JUTXfb.hpp"
#include <aurora/allocation.hpp>

#include <aurora/exception.hpp>
#include <aurora/guest_thread.hpp>
#include <dolphin/gx/GXAurora.h>
#include <exception>
#include <stdexcept>

namespace smgpc::runtime {
    OriginalDisplayLifetime::OriginalDisplayLifetime(render::AuroraWindow& window,
            JKRHeap::Handle heaps, const GXRenderModeObj& mode)
        : _window(window), _mode(mode) {
        const aurora::os::GuestThreadExecutionScope execution;
        const aurora::allocation::HostAllocationScope host;
        if (JUTVideo::getManager() || JUTXfb::getManager() || MainLoopFramework::sManager)
            aurora::throw_host_exception<std::logic_error>("The original display factories already have an owner");
        const std::size_t size = ((std::size_t(mode.fbWidth) + 15) & ~std::size_t(15)) * mode.xfbHeight * 2;
        if (!size)
            aurora::throw_host_exception<std::invalid_argument>("Display buffers require nonzero dimensions");
        auto* displayHeap = JKRSolidHeap::create(static_cast<u32>(size * 3 + 128 * 1024), heaps.get(), false);
        if (!displayHeap) throw std::bad_alloc();
        _heap = displayHeap->adoptNativeOwnership();
        try {
            const JKRHeap::CurrentHeapScope game(*_heap);
            const aurora::allocation::ClientAllocationScope game_routing({true, true});
            if (!JUTDirectPrint::getManager()) _direct = JUTDirectPrint::start();
            for (auto& buffer : _buffers) buffer = new (_heap.get(), 32) u8[size];
            _previous_draw_callback = GXSetDrawDoneCallback(nullptr);
            _draw_callback_installed = true;
            _video = JUTVideo::createManager(&_mode);
            _main_loop = MainLoopFramework::createManager(nullptr, _buffers[0], _buffers[1], _buffers[2], true);
            _window.attach_display(*this);
            _attached = true;
        } catch (...) {
            retire();
            throw;
        }
    }

    OriginalDisplayLifetime::~OriginalDisplayLifetime() { retire(); }

    void OriginalDisplayLifetime::retire() {
        if (!_heap) return;
        const aurora::os::GuestThreadExecutionScope execution;
        const auto retainedHeap = _heap;
        const aurora::allocation::ClientAllocationScope game_routing({true, true});
        if (_attached) {
            _window.detach_display(*this);
            _attached = false;
        }
        AuroraDrainGXCommands();
        if (_draw_callback_installed) {
            GXSetDrawDoneCallback(_previous_draw_callback);
            _draw_callback_installed = false;
        }
        if (_main_loop) {
            if (MainLoopFramework::sManager != _main_loop) std::terminate();
            delete _main_loop;
            MainLoopFramework::sManager = nullptr;
            _main_loop = nullptr;
        }
        const bool had_video = _video != nullptr;
        if (_video) {
            if (JUTVideo::getManager() != _video) std::terminate();
            JUTVideo::destroyManager();
            _video = nullptr;
        }
        if (had_video) {
            VISetBlack(TRUE);
            VISetNextFrameBuffer(nullptr);
            VIFlush();
            VIWaitForRetrace();
        }
        if (auto* direct = JUTDirectPrint::getManager()) direct->changeFrameBuffer(nullptr, 0, 0);
        for (auto& buffer : _buffers) {
            if (buffer) GXDestroyCopyTex(buffer);
            buffer = nullptr;
        }
        AuroraDrainGXCommands();
        if (_direct) {
            if (JUTDirectPrint::getManager() != _direct) std::terminate();
            delete _direct;
            JUTDirectPrint::sDirectPrint = nullptr;
            _direct = nullptr;
        }
    }

    GXRenderModeObj& OriginalDisplayLifetime::render_mode() const { return *_video->getRenderMode(); }
    MainLoopFramework& OriginalDisplayLifetime::main_loop() const { return *_main_loop; }

    void OriginalDisplayLifetime::begin_render(const render::CopyClearState& clear) {
        const aurora::os::GuestThreadExecutionScope execution;
        const auto retainedHeap = _heap;
        const aurora::allocation::ClientAllocationScope game_routing({true, true});
        if (_rendering) aurora::throw_host_exception<std::logic_error>("An original display frame is already open");
        set_copy_clear(clear);
        _main_loop->beginRender();
        _rendering = true;
    }

    void OriginalDisplayLifetime::end_render() {
        const aurora::os::GuestThreadExecutionScope execution;
        const auto retainedHeap = _heap;
        const aurora::allocation::ClientAllocationScope game_routing({true, true});
        if (!_rendering) return;
        _main_loop->endRender();
        _main_loop->endFrame();
        _main_loop->waitForRetrace();
        _rendering = false;
    }

    void OriginalDisplayLifetime::set_copy_clear(const render::CopyClearState& clear) {
        const aurora::os::GuestThreadExecutionScope execution;
        _main_loop->mClearColor = GXColor{clear.color[0], clear.color[1], clear.color[2], clear.color[3]};
        _main_loop->mClearZ = clear.depth;
    }
}
