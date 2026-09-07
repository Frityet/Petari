#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include <aurora/dvd.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    void near(float actual, float expected) {
        require(std::isfinite(actual) && std::abs(actual - expected) < 0.000001f,
                "J2D Simple projection coefficient differs");
    }
    class Logger final : public smgpc::logging::ILogger {
        void write(std::FILE*, std::source_location, smgpc::logging::Level,
                   smgpc::logging::Category, std::string_view) override {}
    };
}

int main() {
    try {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc != nullptr, "SMGPC_REAL_DISC must name the real disc");
        smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original J2D projection"});
        smgpc::render::AuroraRenderer renderer(window);
        require(aurora_dvd_open(disc), "Cannot open the supplied disc");
        struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
        DVDInit();
        smgpc::resource::GameResourceRuntime resources({96U << 20, 32U << 20, 4U << 20});
        Logger logger;
        smgpc::runtime::RuntimeContext runtime(logger, window, resources);
        (void)renderer.begin_frame();
        J2DOrthoGraphSimple simple;
        require(simple.mBounds.getWidth() == MR::getFrameBufferWidth() &&
                    simple.mBounds.getHeight() == MR::getScreenHeight(), "simple graph uses framebuffer viewport dimensions");
        require(simple.mOrtho.getWidth() == MR::getScreenWidth() && simple.mNear == -30000 && simple.mFar == 30000,
                "simple graph retains logical screen extents and original clip signs");
        MR::loadProjectionMtxFor2D();
        std::array<float, 7> projection;
        GXGetProjectionv(projection.data());
        near(projection[0], GX_ORTHOGRAPHIC);
        near(projection[1], 2.0f / MR::getScreenWidth());
        near(projection[2], -1);
        near(projection[3], -2.0f / MR::getScreenHeight());
        near(projection[4], 1);
        near(projection[5], -1.0f / 60000);
        near(projection[6], -0.5f);
        std::cout << "PASS original J2D Simple graph against actual JUT video and logical screen dimensions\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
