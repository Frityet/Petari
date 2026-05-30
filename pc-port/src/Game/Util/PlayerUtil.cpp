#include "Game/Util/PlayerUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>
#include <cstddef>

namespace MR {
    namespace {
        constexpr f32 cRadToDeg = 180.0F / 3.14159265358979323846F;

        void copyPlayerTranslation(TVec3f* pOut) {
            if (pOut == nullptr) {
                return;
            }

            MtxPtr matrix = getPlayerBaseMtx();
            pOut->set(matrix[0][3], matrix[1][3], matrix[2][3]);
        }

        void copyPlayerAxis(TVec3f* pOut, int column, const TVec3f& fallback) {
            if (pOut == nullptr) {
                return;
            }

            MtxPtr matrix = getPlayerBaseMtx();
            pOut->set(matrix[0][column], matrix[1][column], matrix[2][column]);
            const auto length = pOut->length();
            if (length <= 0.001F) {
                pOut->set(fallback);
                return;
            }
            pOut->scale(1.0F / length);
        }
    }  // namespace

    void showPlayer() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->player_system().show_player();
        }
    }

    void hidePlayer() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->player_system().hide_player();
        }
    }

    TVec3f* getPlayerPos() {
        static TVec3f position{};
        copyPlayerTranslation(&position);
        return &position;
    }

    TVec3f* getPlayerCenterPos() {
        return getPlayerPos();
    }

    void setPlayerPos(const TVec3f& position) {
        Mtx matrix{};
        MtxPtr current = getPlayerBaseMtx();
        for (auto row = 0; row < 3; ++row) {
            for (auto column = 0; column < 4; ++column) {
                matrix[row][column] = current[row][column];
            }
        }
        matrix[0][3] = position.x;
        matrix[1][3] = position.y;
        matrix[2][3] = position.z;
        setPlayerBaseMtx(matrix);
    }

    TVec3f* getPlayerRotate() {
        static TVec3f rotation{};
        TVec3f front{};
        getPlayerFrontVec(&front);
        rotation.set(0.0F, std::atan2(front.x, front.z) * cRadToDeg, 0.0F);
        return &rotation;
    }

    TVec3f* getPlayerVelocity() {
        static TVec3f velocity{};
        velocity.set(0.0F, 0.0F, 0.0F);
        return &velocity;
    }

    TVec3f* getPlayerGravity() {
        static TVec3f gravity{0.0F, -1.0F, 0.0F};
        return &gravity;
    }

    f32 calcDistanceToPlayer(const TVec3f& position) {
        return position.distance(*getPlayerPos());
    }

    void getPlayerUpVec(TVec3f* pOut) {
        copyPlayerAxis(pOut, 1, TVec3f{0.0F, 1.0F, 0.0F});
    }

    void getPlayerFrontVec(TVec3f* pOut) {
        copyPlayerAxis(pOut, 2, TVec3f{0.0F, 0.0F, 1.0F});
    }

    void getPlayerSideVec(TVec3f* pOut) {
        copyPlayerAxis(pOut, 0, TVec3f{1.0F, 0.0F, 0.0F});
    }

    void setPlayerBaseMtx(MtxPtr matrix) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
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

        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->player_system().has_base_matrix()) {
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
