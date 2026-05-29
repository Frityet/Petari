#include "Game/LiveActor/Nerve.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <revolution.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    int g_pre_retrace = -1;
    int g_post_retrace = -1;

    void on_pre_retrace(u32 count) {
        g_pre_retrace = static_cast<int>(count);
    }

    void on_post_retrace(u32 count) {
        g_post_retrace = static_cast<int>(count);
    }

    void test_revolution_headers_and_input_defaults() {
        static_assert(sizeof(u8) == 1U);
        static_assert(sizeof(u16) == 2U);
        static_assert(sizeof(u32) == 4U);
        static_assert(sizeof(s32) == 4U);
        static_assert(NAND_MAX_PATH == 64U);

        auto type = u32 {0xFFFFFFFFU};
        require(WPADProbe(0, &type) == FALSE, "WPADProbe should report disconnected without a runtime context");
        require(type == 0U, "WPADProbe should zero the controller type when disconnected");

        auto status = KPADStatus {};
        require(KPADRead(0, &status, 1U) == 0, "KPADRead should not synthesize samples without a runtime context");
    }

    void test_aurora_vi_retrace_and_framebuffer_state() {
        VIInit();
        VIConfigure(&GXNtsc480IntDf);
        require(VIGetTvFormat() == VI_NTSC, "VI should expose NTSC from GXNtsc480IntDf");
        require(VIGetScanMode() == VI_INTERLACE, "VI should expose interlaced scan mode");
        require(VIGetRetraceCount() == 0U, "VI retrace count should reset to zero");

        auto framebuffer = std::array<std::uint8_t, 32U> {};
        VISetNextFrameBuffer(framebuffer.data());
        require(VIGetNextFrameBuffer() == framebuffer.data(), "VI next framebuffer should be stored");

        g_pre_retrace = -1;
        g_post_retrace = -1;
        require(VISetPreRetraceCallback(on_pre_retrace) == nullptr, "first VI pre callback install should return null");
        require(VISetPostRetraceCallback(on_post_retrace) == nullptr, "first VI post callback install should return null");
        VIWaitForRetrace();
        require(VIGetRetraceCount() == 1U, "VIWaitForRetrace should advance the retrace count");
        require(VIGetCurrentFrameBuffer() == framebuffer.data(), "VIWaitForRetrace should publish the next framebuffer");
        require(g_pre_retrace == 0 && g_post_retrace == 1, "VI retrace callbacks should receive pre/post counts");
        VISetPreRetraceCallback(nullptr);
        VISetPostRetraceCallback(nullptr);
    }

    void test_aurora_dvd_requires_disc_image() {
        aurora_dvd_close();
        require(!aurora_dvd_open(nullptr), "aurora_dvd_open should reject a null disc path");
        require(!aurora_dvd_open("/definitely/not/a/smg/disc.rvz"), "aurora_dvd_open should reject missing disc images");
        DVDInit();
        require(DVDGetDriveStatus() == DVD_STATE_NO_DISK, "DVD should report no disk until Aurora opens a disc image");
        require(DVDConvertPathToEntrynum("/ObjectData") < 0, "DVD path lookup should fail without an open disc image");

        auto file = DVDFileInfo {};
        require(DVDOpen("/ObjectData/Mario.arc", &file) == FALSE, "DVDOpen should fail without an open disc image");
    }

    void test_aurora_os_cache_and_gx_copy_smoke() {
        auto cache_bytes = std::array<std::uint8_t, 64U> {};
        DCFlushRange(cache_bytes.data(), static_cast<u32>(cache_bytes.size()));
        DCInvalidateRange(cache_bytes.data(), static_cast<u32>(cache_bytes.size()));
        DCZeroRange(cache_bytes.data(), static_cast<u32>(cache_bytes.size()));
        require(cache_bytes[0] == 0U && cache_bytes.back() == 0U, "DCZeroRange should clear its byte range");

        alignas(32) auto fifo = std::array<std::uint8_t, 32U * 1024U> {};
        require(GXInit(fifo.data(), static_cast<u32>(fifo.size())) != nullptr, "GXInit should return a FIFO object");
        GXSetCopyClear(GXColor {.r = 0U, .g = 0U, .b = 0U, .a = 255U}, GX_MAX_Z24);
        GXSetDispCopySrc(0U, 0U, 640U, 456U);
        GXSetDispCopyDst(640U, 456U);
        GXCopyDisp(nullptr, GX_TRUE);
    }

    void test_pc_port_nand_storage_smoke() {
        auto nand = smgpc::runtime::NandFileSystemService {};
        const auto payload = std::array<std::uint8_t, 4U> {1U, 2U, 3U, 4U};
        nand.write_file("save/banner.bin", std::span<const std::uint8_t>(payload), 0x3CU, 0U);

        const auto normalized = nand.normalize_path("save/banner.bin");
        require(normalized.starts_with(smgpc::runtime::NandFileSystemService::title_data_root()),
                "relative NAND paths should live under the title data root");
        require(nand.exists("save/banner.bin"), "NAND write should make the file visible");

        const auto readback = nand.read_file("save/banner.bin");
        require(readback.has_value() && *readback == std::vector<std::uint8_t>(payload.begin(), payload.end()),
                "NAND readback should match written bytes");
        require(nand.rename("save/banner.bin", "save/banner-new.bin") == NAND_RESULT_OK, "NAND rename should succeed");
        require(!nand.exists("save/banner.bin") && nand.exists("save/banner-new.bin"), "NAND rename should move the file");
        require(nand.erase("save/banner-new.bin"), "NAND erase should remove the file");

        const auto check = nand.check(1U, 1U);
        require(check.result == NAND_RESULT_OK && check.free_blocks > 0U && check.free_inodes > 0U,
                "NAND quota check should report free space");
    }

    struct SpineProbeState {
        int first_executions = 0;
        int second_executions = 0;
        const Nerve *second_nerve = nullptr;
    };

    class SpineProbeFirstNerve final : public Nerve {
    public:
        void execute(Spine *spine) const override {
            auto *state = static_cast<SpineProbeState *>(spine->mExecutor);
            ++state->first_executions;
            spine->setNerve(state->second_nerve);
        }
    };

    class SpineProbeSecondNerve final : public Nerve {
    public:
        void execute(Spine *spine) const override {
            auto *state = static_cast<SpineProbeState *>(spine->mExecutor);
            ++state->second_executions;
        }
    };

    void test_spine_pending_nerve_runs_next_tick() {
        const auto first = SpineProbeFirstNerve {};
        const auto second = SpineProbeSecondNerve {};
        auto state = SpineProbeState {
            .second_nerve = &second,
        };
        auto spine = Spine(&state, &first);

        require(spine.getCurrentNerve() == &first, "Spine should start on its initial nerve");
        spine.update();
        require(state.first_executions == 1, "first nerve should execute on the first update");
        require(state.second_executions == 0, "queued nerve should not execute in the same update");
        require(spine.getCurrentNerve() == &first, "queued nerve should not appear current before it executes");

        spine.update();
        require(state.second_executions == 1, "queued nerve should execute on the next update");
        require(spine.getCurrentNerve() == &second, "executed nerve should become current after its first tick");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    const auto tests = std::array {
        TestCase {"revolution headers and input defaults", test_revolution_headers_and_input_defaults},
        TestCase {"Aurora VI retrace/framebuffer state", test_aurora_vi_retrace_and_framebuffer_state},
        TestCase {"Aurora DVD requires disc image", test_aurora_dvd_requires_disc_image},
        TestCase {"Aurora OS cache and GX copy smoke", test_aurora_os_cache_and_gx_copy_smoke},
        TestCase {"pc-port NAND storage smoke", test_pc_port_nand_storage_smoke},
        TestCase {"Spine pending nerve runs next tick", test_spine_pending_nerve_runs_next_tick},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " Aurora-native test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " Aurora-native test(s) passed\n";
    return 0;
}
