#include "JSystem/JUtility/JUTVideo.hpp"
#include "JSystem/JUtility/JUTXfb.hpp"
#include "JSystem/JUtility/JUTDirectPrint.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "Game/System/MainLoopFramework.hpp"
#include "compat/DrawSyncManagerLifetime.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "render/RendererService.hpp"
#include "runtime/jut/OriginalDisplayLifetime.hpp"

#include <aurora/aurora.h>
#include <aurora/guest_thread.hpp>
#include <aurora/vi.hpp>
#include <dolphin/gx/GXAurora.h>

#include "../aurora/lib/gx/gx.hpp"
#include "../aurora/lib/gfx/frame.hpp"
#include "../aurora/lib/webgpu/gpu.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct AuroraLifetime { ~AuroraLifetime() { aurora_shutdown(); } };

unsigned sPreviousPre = 0;
unsigned sPreviousPost = 0;
unsigned sPreviousDraw = 0;
void previousPre(u32) { ++sPreviousPre; }
void previousPost(u32) { ++sPreviousPost; }
void previousDraw() { ++sPreviousDraw; }

void receiveRetrace(JUTVideo& video) {
    OSMessage message;
    while (OSReceiveMessage(video.getMessageQueue(), &message, OS_MESSAGE_NOBLOCK)) {}
    require(OSReceiveMessage(video.getMessageQueue(), &message, OS_MESSAGE_BLOCK),
            "original JUTVideo post callback must publish its actual queue message");
    require(reinterpret_cast<std::uintptr_t>(message) == VIGetRetraceCount(),
            "the original retrace message must preserve the complete count");
}

void exercise() {
    AuroraConfig config{};
    config.appName = "Original JUTVideo owner proof";
#if defined(__APPLE__)
    config.desiredBackend = BACKEND_METAL;
#else
    config.desiredBackend = BACKEND_VULKAN;
#endif
    config.allowCpuAdapter = true;
    config.windowWidth = 128;
    config.windowHeight = 96;
    config.vsync = false;
    config.pauseOnFocusLost = false;
    config.logLevel = LOG_WARNING;
    const auto info = aurora_initialize(0, nullptr, &config);
    const AuroraLifetime auroraLifetime;
    require(info.backend == config.desiredBackend, "actual requested graphics backend is required");
    const aurora::os::GuestThreadExecutionScope execution;
    alignas(32) std::array<u8, 64 * 1024> fifoStorage{};
    GXInit(fifoStorage.data(), fifoStorage.size());
    GXRenderModeObj mode = GXNtsc480IntDf;
    mode.viTVmode = VI_TVMODE_NTSC_PROG;
    mode.fbWidth = mode.viWidth = 128;
    mode.efbHeight = mode.xfbHeight = mode.viHeight = 96;
    alignas(32) std::array<u8, 128 * 96 * 2> a{}, b{}, c{};
    auto* const direct = JUTDirectPrint::start();
    require(direct != nullptr, "actual original direct-print owner must exist before video callbacks");
    VIInit();
    VISetPreRetraceCallback(previousPre);
    VISetPostRetraceCallback(previousPost);
    const auto outerDraw = GXSetDrawDoneCallback(previousDraw);
    auto* const video = JUTVideo::createManager(&mode);
    require(video == JUTVideo::createManager(&mode) && video->getRenderMode() == &mode,
            "original video factory must retain one actual owner and borrowed render-mode record");
    auto* const xfb = JUTXfb::createManager(a.data(), b.data(), c.data());
    require(xfb->getBufferNum() == 3 && xfb->getDrawingXfb() == nullptr &&
                xfb->getDrawnXfb() == nullptr && xfb->getDisplayingXfb() == nullptr,
            "original XFB constructor must own three borrowed buffers and cleared indices");
    xfb->setDrawnXfbIndex(0);
    receiveRetrace(*video);
    require(aurora::vi::scanout_state().black && VIGetCurrentFrameBuffer() == nullptr,
            "first original startup retrace must stay black");
    receiveRetrace(*video);
    require(aurora::vi::scanout_state().black && VIGetCurrentFrameBuffer() == nullptr,
            "second original startup retrace must stay black");
    receiveRetrace(*video);
    require(VIGetCurrentFrameBuffer() == a.data() && xfb->getDisplayingXfbIndex() == 0,
            "original triple-buffer pre callback must publish the drawn framebuffer");
    receiveRetrace(*video);
    require(!aurora::vi::scanout_state().black && direct->mFrameBuffer == reinterpret_cast<u16*>(a.data()),
            "subsequent original callback must unblack VI and update direct-print backing");
    require(JUTVideo::getVideoInterval() != 0 && JUTVideo::getVideoLastTick() != 0,
            "actual retrace clock must update original JUT timing fields");

    // Hold the original draw-done state with its real FIFO command pending.
    // VI must continue scanning A until that callback has actually completed.
    aurora_update();
    require(aurora_begin_frame(), "actual GPU recording frame required");
    GXFifoObj fifo;
    require(GXGetCPUFifo(&fifo), "actual CPU FIFO pointer must be available");
    void* readPointer;
    void* writePointer;
    GXGetFifoPtrs(&fifo, &readPointer, &writePointer);
    GXEnableBreakPt(writePointer);
    JUTVideo::drawDoneStart();
    xfb->setDrawnXfbIndex(1);
    receiveRetrace(*video);
    require(xfb->getDisplayingXfbIndex() == 0 && VIGetCurrentFrameBuffer() == a.data(),
            "pending original draw completion must gate the next XFB exchange");
    GXDisableBreakPt();
    GXDrawDone();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (xfb->getDisplayingXfbIndex() != 1) {
        require(std::chrono::steady_clock::now() < deadline,
                "the SDK XFB getter must admit VI interrupts during an original guest polling loop");
    }
    require(xfb->getDisplayingXfbIndex() == 1 && VIGetCurrentFrameBuffer() == b.data(),
            "real GX completion callback must release the original XFB exchange");
    // This owner proof checks original VI/GX scheduling, not display-copy
    // rendering; black the output before submitting its empty host frame.
    JUTXfb::destroyManager();
    VISetBlack(TRUE);
    VIFlush();
    VIWaitForRetrace();
    aurora_end_frame();

    // Retail JUTVideo lasts until process exit and does not restore its GX
    // callback. The enclosing native owner must quiesce and retire that
    // borrowed registration before releasing the actual SDK objects.
    GXDrawDone();
    require(GXSetDrawDoneCallback(previousDraw) == JUTVideo::drawDoneCallback,
            "native lifetime must retire the actual borrowed original GX callback");
    JUTVideo::destroyManager();
    require(JUTXfb::getManager() == nullptr && JUTVideo::getManager() == nullptr,
            "actual SDK factories must clear their manager pointers on retirement");
    const auto pre = sPreviousPre, post = sPreviousPost;
    VIWaitForRetrace();
    require(sPreviousPre == pre + 1 && sPreviousPost == post + 1,
            "original JUT destructor must restore the preceding VI callback registrations");
    GXDrawDone();
    require(sPreviousDraw != 0, "restored GX callback must remain operational after JUT retirement");
    GXSetDrawDoneCallback(outerDraw);
    VISetPreRetraceCallback(nullptr);
    VISetPostRetraceCallback(nullptr);
    aurora::vi::shutdown();
    direct->changeFrameBuffer(nullptr, 0, 0);
    delete direct;
    JUTDirectPrint::sDirectPrint = nullptr;
}

void expectSelectedPixels(const smgpc::render::AuroraWindow& window, const GXRenderModeObj& mode) {
    const auto scanout = aurora::vi::scanout_state();
    const auto selected = aurora::gx::select_display_copy(scanout.initialized, scanout.black, scanout.frame_buffer);
    require(selected.supported && selected.drawVideo && selected.copy.handle,
            "VI must select a real retained display copy for its actual framebuffer");
    const auto logical = aurora::gx::logical_fb_size();
    const auto physical = window.framebuffer_size();
    require(logical.x == mode.fbWidth && logical.y == mode.efbHeight,
            "the actual VI render mode must define the logical display dimensions");
    // AuroraWindow uses the stretch policy: GX logical copy dimensions are
    // scaled to the physical render target, including host display density.
    const auto expectedWidth = static_cast<u32>(std::lround(
        float(mode.fbWidth) * float(physical.width) / float(logical.x)));
    const auto copyHeight = GXGetNumXfbLines(mode.efbHeight, GXGetYScaleFactor(mode.efbHeight, mode.xfbHeight));
    const auto expectedHeight = static_cast<u32>(std::lround(
        float(copyHeight) * float(physical.height) / float(logical.y)));
    u32 width = 0, height = 0, stride = 0;
    require(AuroraReadDisplayCopyRGBA8(nullptr, 0, &width, &height, &stride),
            "the original GX display-copy sequence must expose GPU readback dimensions");
    const auto& texture = selected.copy.handle;
    require(width == expectedWidth && height == expectedHeight && stride == width * 4 &&
                texture->size.width == expectedWidth && texture->size.height == expectedHeight,
            "both completed and VI-selected copies must preserve the configured logical-to-physical scale");
    aurora::gfx::gpu_synchronize();
    const wgpu::BufferDescriptor descriptor{
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, .size = 256};
    auto buffer = aurora::webgpu::g_device.CreateBuffer(&descriptor);
    auto encoder = aurora::webgpu::g_device.CreateCommandEncoder();
    const wgpu::TexelCopyTextureInfo source{
        .texture = texture->texture, .origin = {expectedWidth / 2, expectedHeight / 2, 0}};
    const wgpu::TexelCopyBufferInfo destination{
        .layout = {.bytesPerRow = 256, .rowsPerImage = 1}, .buffer = buffer};
    constexpr wgpu::Extent3D extent{1, 1, 1};
    encoder.CopyTextureToBuffer(&source, &destination, &extent);
    const auto command = encoder.Finish();
    aurora::webgpu::g_queue.Submit(1, &command);
    std::atomic<bool> finished{false};
    bool success = false;
    buffer.MapAsync(wgpu::MapMode::Read, 0, 256, wgpu::CallbackMode::AllowSpontaneous,
                    [&](wgpu::MapAsyncStatus status, wgpu::StringView) {
        success = status == wgpu::MapAsyncStatus::Success;
        finished.store(true, std::memory_order_release);
    });
    while (!finished.load(std::memory_order_acquire)) {
        aurora::webgpu::g_instance.ProcessEvents();
        std::this_thread::yield();
    }
    require(success, "selected VI texture GPU readback must complete");
    const auto* pixel = static_cast<const u8*>(buffer.GetConstMappedRange(0, 256));
    const bool bgra = texture->format == wgpu::TextureFormat::BGRA8Unorm ||
                      texture->format == wgpu::TextureFormat::BGRA8UnormSrgb;
    const auto red = pixel[bgra ? 2 : 0], green = pixel[1], blue = pixel[bgra ? 0 : 2];
    const bool matches = red >= 26 && red <= 34 && green >= 56 && green <= 64 && blue >= 196 && blue <= 204;
    std::cout << "VI-selected GPU copy " << width << 'x' << height << " (logical "
              << mode.fbWidth << 'x' << mode.efbHeight << "), pixel "
              << unsigned(red) << ',' << unsigned(green) << ',' << unsigned(blue) << '\n';
    buffer.Unmap();
    require(matches, "actual MainLoop clear and VI-selected GX copy must preserve the selected color");
}

void exerciseDisplayFrames() {
    smgpc::render::WindowConfiguration config{};
    config.title = "Original MainLoop display lifetime";
    config.width = 128;
    config.height = 96;
    smgpc::render::AuroraWindow window(config);
    smgpc::render::AuroraRenderer renderer(window);
    const aurora::os::GuestThreadExecutionScope execution;
    auto heaps = smgpc::compat::JkrHeapRuntime::create(4U * 1024U * 1024U);
    const auto freeBytes = heaps->root_heap().getTotalFreeSize();
    GXRenderModeObj mode = GXNtsc480IntDf;
    mode.viTVmode = VI_TVMODE_NTSC_PROG;
    mode.fbWidth = mode.viWidth = 128;
    mode.efbHeight = mode.xfbHeight = mode.viHeight = 96;
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        {
            smgpc::compat::DrawSyncManagerLifetime drawSync(heaps);
            smgpc::runtime::OriginalDisplayLifetime display(window, heaps, mode);
            require(&display.main_loop() == MainLoopFramework::sManager &&
                        &display.render_mode() == JUTVideo::getManager()->getRenderMode(),
                    "frame source must use the original factory identities and their actual render mode");
            if (cycle == 0) {
                bool rejected = false;
                try { window.shutdown(); }
                catch (const std::logic_error&) { rejected = true; }
                require(rejected, "an attached display must prevent premature GPU/VI service shutdown");
            }
            renderer.set_copy_clear({.color = {30, 60, 200, 255}});
            for (unsigned frame = 0; frame < 6; ++frame) {
                require(window.poll_events(), "display window must remain available");
                (void)renderer.begin_frame();
                renderer.end_frame();
            }
            auto* xfb = JUTXfb::getManager();
            require(xfb->getDrawingXfb() && xfb->getDrawnXfb() && xfb->getDisplayingXfb(),
                    "actual MainLoop must advance all three XFB roles");
            require(VIGetCurrentFrameBuffer() == xfb->getDisplayingXfb() && !aurora::vi::scanout_state().black,
                    "actual retrace callback must scan the completed original display buffer");
            expectSelectedPixels(window, mode);
            // Teardown must also complete an open frame before retiring the
            // borrowed callback objects and buffers.
            if (cycle == 1) (void)renderer.begin_frame();
        }
        require(MainLoopFramework::sManager == nullptr && JUTXfb::getManager() == nullptr &&
                    JUTVideo::getManager() == nullptr && JUTDirectPrint::getManager() == nullptr,
                "native display retirement must clear every owned original factory");
        require(VIGetCurrentFrameBuffer() == nullptr && aurora::vi::scanout_state().black,
                "VI must stop borrowing retired display storage");
        require(heaps->root_heap().getTotalFreeSize() == freeBytes,
                "three actual XFBs, display objects and draw-sync allocations must return to the root heap");
        if (cycle == 1) renderer.end_frame();
    }
    std::cout << "[ok] actual MainLoop frames, GPU display-copy pixels and three complete display heap retirements\n";
}
}
int main() {
    try {
        exercise();
        exerciseDisplayFrames();
        std::cout << "[ok] actual original JUTVideo/Xfb factories, retrace queue, draw-completion gating and callback retirement\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] original JUTVideo: " << error.what() << '\n';
        return 1;
    }
}
