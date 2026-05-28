#include "Game/Util/PlayerUtil.hpp"

#include "runtime/RuntimeContext.hpp"

#include <cstddef>

namespace MR {
    void hidePlayer() {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            runtime->player_system().hide_player();
        }
    }

    void setPlayerBaseMtx(MtxPtr matrix) {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            runtime->player_system().set_base_matrix(matrix);
        }
    }

    MtxPtr getPlayerBaseMtx() {
        static Mtx matrix{};
        matrix[0][0] = 1.0F;
        matrix[0][1] = 0.0F;
        matrix[0][2] = 0.0F;
        matrix[0][3] = 0.0F;
        matrix[1][0] = 0.0F;
        matrix[1][1] = 1.0F;
        matrix[1][2] = 0.0F;
        matrix[1][3] = 0.0F;
        matrix[2][0] = 0.0F;
        matrix[2][1] = 0.0F;
        matrix[2][2] = 1.0F;
        matrix[2][3] = 0.0F;

        if (const auto* runtime = smgpc::compat::RuntimeContext::try_instance(); runtime != nullptr && runtime->player_system().has_base_matrix()) {
            const auto values = runtime->player_system().base_matrix();
            auto index = std::size_t{};
            for (auto row = 0U; row < 3U; ++row) {
                for (auto column = 0U; column < 4U; ++column) {
                    matrix[row][column] = values[index++];
                }
            }
        }

        return matrix;
    }

    void startBckPlayer(const char*, const char*) {
    }

    f32 getBckFrameMaxPlayer(const char*) {
        return 180.0F;
    }

    bool isBckStoppedPlayer() {
        return true;
    }

    void initPlayerAfterOpeningDemo() {
    }
}  // namespace MR
