#include "compat/MetrowerksStdCompat.hpp"

#include "JSystem/J3DGraphBase/J3DTexture.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void near(float actual, float expected, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.0001F) {
            throw std::runtime_error(std::string(message) + ": " + std::to_string(actual) +
                                     " != " + std::to_string(expected));
        }
    }

    template <typename T>
    std::array<unsigned char, sizeof(T)> bytes(const T& object) {
        std::array<unsigned char, sizeof(T)> result{};
        std::memcpy(result.data(), &object, result.size());
        return result;
    }

    J3DTexMtxInfo make_info() {
        J3DTexMtxInfo result;
        std::memset(&result, 0xA7, sizeof(result));
        result.mProjection = GX_MTX2x4;
        result.mInfo = J3DTexMtxMode_EnvmapBasic;
        result.mCenter = {1.25F, -2.5F, 3.75F};
        result.mSRT.mScaleX = 2;
        result.mSRT.mScaleY = 3;
        result.mSRT.mRotation = -1234;
        result.mSRT.mTranslationX = 4;
        result.mSRT.mTranslationY = 5;
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                result.mEffectMtx[row][column] = static_cast<float>(row * 4 + column) - 7.5F;
            }
        }
        return result;
    }

    void test_srt_assignment_preserves_rotation_word() {
        auto info = make_info();
        J3DTextureSRTInfo destination;
        std::memset(&destination, 0x31, sizeof(destination));
        destination = info.mSRT;
        require(bytes(destination) == bytes(info.mSRT),
                "SRT assignment copies all five fields and both bytes following rotation");
        require(destination.mScaleX == 2 && destination.mScaleY == 3 && destination.mRotation == -1234 &&
                    destination.mTranslationX == 4 && destination.mTranslationY == 5,
                "SRT assignment preserves actual typed values");
        const auto before = bytes(destination);
        destination.operator=(destination);
        require(bytes(destination) == before, "SRT self-assignment preserves the complete original word");
    }

    void test_info_assignment_preserves_unwritten_bytes() {
        auto source = make_info();
        J3DTexMtxInfo destination;
        std::memset(&destination, 0x31, sizeof(destination));
        destination = source;
        auto expected = bytes(source);
        expected[offsetof(J3DTexMtxInfo, field_0x2)] = 0x31;
        expected[offsetof(J3DTexMtxInfo, field_0x3)] = 0x31;
        require(bytes(destination) == expected,
                "original info assignment leaves bytes2/3 untouched and copies the remaining fields");

        J3DTexMtx actual(source);
        require(actual.getTexMtxInfo().mProjection == source.mProjection &&
                    actual.getTexMtxInfo().mInfo == source.mInfo &&
                    bytes(actual.getTexMtxInfo().mSRT) == bytes(source.mSRT),
                "real texture-matrix construction uses the recovered assignment");
    }

    void test_effect_matrix_extends_affine_row() {
        auto info = make_info();
        const auto original = bytes(info);
        Mtx affine{{2, 3, 4, 5}, {-6, 7, 8, 9}, {10, 11, -12, 13}};
        info.setEffectMtx(affine);
        require(std::memcmp(&info, original.data(), offsetof(J3DTexMtxInfo, mEffectMtx)) == 0,
                "effect-matrix update leaves texture flags, center and SRT untouched");
        require(std::memcmp(info.mEffectMtx, affine, sizeof(affine)) == 0,
                "effect-matrix update preserves all twelve supplied affine entries");
        require(info.mEffectMtx[3][0] == 0 && info.mEffectMtx[3][1] == 0 &&
                    info.mEffectMtx[3][2] == 0 && info.mEffectMtx[3][3] == 1,
                "effect-matrix update appends the homogeneous row 0,0,0,1");

        J3DTexMtx actual(info);
        Mtx replacement{{1, 0, 0, 17}, {0, 1, 0, 19}, {0, 0, 1, 23}};
        actual.setEffectMtx(replacement);
        require(actual.getTexMtxInfo().mEffectMtx[0][3] == 17 &&
                    actual.getTexMtxInfo().mEffectMtx[3][3] == 1,
                "real J3DTexMtx forwarding updates the owned original effect matrix");
    }

    void require_matrix(const Mtx matrix, const std::array<float, 12>& expected, std::string_view message) {
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                near(matrix[row][column], expected[row * 4 + column], message);
            }
        }
    }

    void test_actual_calc_and_post_calc_contracts() {
        J3DTexMtx actual;
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                require(actual.getTexMtxInfo().mEffectMtx[row][column] == (row == column ? 1.0F : 0.0F),
                        "retail default texture info contains the full identity effect matrix");
            }
        }
        auto& info = actual.getTexMtxInfo();
        info.mInfo = J3DTexMtxMode_EnvmapBasic;
        info.mCenter = {0, 0, 0};
        info.mSRT.mScaleX = 2;
        info.mSRT.mScaleY = 3;
        info.mSRT.mRotation = 0;
        info.mSRT.mTranslationX = 4;
        info.mSRT.mTranslationY = 5;
        Mtx camera{{1, 0, 0, 10}, {0, 1, 0, 20}, {0, 0, 1, 30}};
        actual.calc(camera);
        // Ordinary texture calculation applies SRT to the incoming translated
        // coordinates: 2*10+4=24 and 3*20+5=65.
        require_matrix(actual.getMtx(), {2, 0, 0, 24, 0, 3, 0, 65, 0, 0, 1, 30},
                       "original ordinary environment texture matrix");
        actual.calcPostTexMtx(camera);
        // This original post-matrix mode keeps its own SRT because the shape
        // path supplies the per-envelope matrix separately.
        require_matrix(actual.getMtx(), {2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 1, 0},
                       "original post environment texture matrix");
    }
}  // namespace

int main() {
    try {
        test_srt_assignment_preserves_rotation_word();
        test_info_assignment_preserves_unwritten_bytes();
        test_effect_matrix_extends_affine_row();
        test_actual_calc_and_post_calc_contracts();
        std::cout << "4/4 original J3D texture-matrix groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
