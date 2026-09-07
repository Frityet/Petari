#include "Game/Player/MarioActor.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoUtilCompat.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace smgpc::compat {
    namespace {
        thread_local smgpc::runtime::PlayerSystemService *sPlayerSystemOverride = nullptr;
    }

    ScopedPlayerSystemServiceOverride::ScopedPlayerSystemServiceOverride(
        smgpc::runtime::PlayerSystemService &service)
        : _previous(std::exchange(sPlayerSystemOverride, &service)) {
    }

    ScopedPlayerSystemServiceOverride::~ScopedPlayerSystemServiceOverride() {
        sPlayerSystemOverride = _previous;
    }

    [[nodiscard]] smgpc::runtime::PlayerSystemService *active_player_system_for_player_util() {
        if (sPlayerSystemOverride != nullptr) {
            return sPlayerSystemOverride;
        }
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr ? &runtime->player_system() : nullptr;
    }
}  // namespace smgpc::compat

namespace MR {
    namespace {
        [[nodiscard]] LiveActor* activePlayerActor() {
            auto* player = smgpc::compat::active_player_system_for_player_util();
            return player != nullptr ? player->attached_actor() : nullptr;
        }

        [[nodiscard]] s32 activePlayerElementMode() {
            auto* player = smgpc::compat::active_player_system_for_player_util();
            if (player == nullptr || player->attached_actor() == nullptr) {
                throw std::logic_error(
                    "Player element mode requires an attached player actor.");
            }
            const auto mode = player->player_element_mode();
            if (!mode.has_value()) {
                throw std::logic_error(
                    "The attached player actor does not expose a retail element-mode capability.");
            }
            return *mode;
        }

        void copyPlayerAxis(TVec3f* pOut, int column) {
            if (pOut == nullptr) {
                return;
            }

            MtxPtr matrix = getPlayerBaseMtx();
            if (matrix == nullptr) {
                throw std::logic_error("Player base-matrix state is unavailable.");
            }
            pOut->set(matrix[0][column], matrix[1][column], matrix[2][column]);
            const auto length = pOut->length();
            if (length <= 0.001F) {
                throw std::logic_error("Player base matrix has a degenerate axis.");
            }
            pOut->scale(1.0F / length);
        }
    }  // namespace

    void showPlayer() {
        auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player == nullptr || player->attached_actor() == nullptr) {
            throw std::logic_error("Cannot show an unavailable player actor.");
        }
        player->show_player();
    }

    void hidePlayer() {
        auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player == nullptr || player->attached_actor() == nullptr) {
            throw std::logic_error("Cannot hide an unavailable player actor.");
        }
        player->hide_player();
    }

    TVec3f* getPlayerPos() {
        auto* actor = activePlayerActor();
        return actor != nullptr ? &actor->mPosition : nullptr;
    }

    TVec3f* getPlayerCenterPos() {
        return nullptr;
    }

    void setPlayerPos(const TVec3f& position) {
        Mtx matrix{};
        MtxPtr current = getPlayerBaseMtx();
        if (current == nullptr) {
            throw std::logic_error("Cannot position an unavailable player actor.");
        }
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
        auto* actor = activePlayerActor();
        return actor != nullptr ? &actor->mRotation : nullptr;
    }

    TVec3f* getPlayerVelocity() {
        auto* actor = activePlayerActor();
        return actor != nullptr ? &actor->mVelocity : nullptr;
    }

    const TVec3f* getPlayerGravity() {
        auto* actor = activePlayerActor();
        return actor != nullptr ? &actor->mGravity : nullptr;
    }

    f32 calcDistanceToPlayer(const TVec3f& position) {
        const auto* player_position = getPlayerPos();
        if (player_position == nullptr) {
            throw std::logic_error("Player position is unavailable.");
        }
        return position.distance(*player_position);
    }

    bool isNearPlayerAnyTime(const LiveActor* actor, f32 distance) {
        const auto* player = activePlayerActor();
        if (actor == nullptr || player == nullptr) {
            throw std::logic_error(
                "Player-distance queries require a host actor and attached player actor.");
        }
        return actor->mPosition.squared(player->mPosition) < (distance * distance);
    }

    bool isPlayerElementMode(s32 mode) {
        return activePlayerElementMode() == mode;
    }

    bool isPlayerElementModeTornado() {
        return isPlayerElementMode(9);
    }

    bool isPlayerElementModeInvincible() {
        return isPlayerElementMode(1);
    }

    bool isPlayerElementModeBee() {
        return isPlayerElementMode(4);
    }

    bool isPlayerElementModeHopper() {
        return isPlayerElementMode(5);
    }

    bool isPlayerElementModeTeresa() {
        return isPlayerElementMode(6);
    }

    bool isPlayerElementModeIce() {
        return isPlayerElementMode(3);
    }

    bool isPlayerElementModeNormal() {
        return isPlayerElementMode(0);
    }

    void getPlayerUpVec(TVec3f* pOut) {
        if (pOut == nullptr) {
            return;
        }
        const auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player != nullptr && player->copy_actor_up_vector(pOut)) {
            return;
        }
        copyPlayerAxis(pOut, 1);
    }

    void getPlayerFrontVec(TVec3f* pOut) {
        if (pOut == nullptr) {
            return;
        }
        const auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player != nullptr && player->copy_actor_front_vector(pOut)) {
            return;
        }
        copyPlayerAxis(pOut, 2);
    }

    void getPlayerSideVec(TVec3f* pOut) {
        if (pOut == nullptr) {
            return;
        }
        const auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player != nullptr && player->copy_actor_side_vector(pOut)) {
            return;
        }
        copyPlayerAxis(pOut, 0);
    }

    void setPlayerBaseMtx(MtxPtr matrix) {
        auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player == nullptr || player->attached_actor() == nullptr || matrix == nullptr) {
            throw std::logic_error("Cannot set the base matrix of an unavailable player actor.");
        }
        player->set_base_matrix(matrix);
    }

    MtxPtr getPlayerBaseMtx() {
        static Mtx matrix{};
        const auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player != nullptr) {
            if (const auto original = player->actor_base_matrix(); original != nullptr) {
                return original;
            }
        }
        if (player == nullptr || player->attached_actor() == nullptr || !player->has_base_matrix()) {
            return nullptr;
        }

        const auto values = player->base_matrix();
        auto index = std::size_t{};
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                matrix[row][column] = values[index++];
            }
        }

        return matrix;
    }

    void startBckPlayer(const char* pName, const char* pParam2) {
        if (pParam2 != nullptr) {
            MarioAccess::changeAnimationE(pName, pParam2);
        } else {
            MarioAccess::changeAnimationE(pName, pName);
        }
    }

    s16 getBckFrameMaxPlayer(const char* pName) {
        return MR::getBckFrameMax(MarioAccess::getPlayerActor(), pName);
    }

    bool isBckStoppedPlayer() {
        return MR::isBckStopped(MarioAccess::getPlayerActor());
    }

    void initPlayerAfterOpeningDemo() {
        auto *player = smgpc::compat::active_player_system_for_player_util();
        auto *actor = player != nullptr ? player->attached_actor() : nullptr;
        if (player == nullptr || actor == nullptr) {
            throw std::logic_error("Opening-demo player teardown requires an attached player actor.");
        }
        smgpc::compat::release_puppetable_demo_control(true);
        player->finish_opening_demo();
        MR::startBckPlayer("Wait", nullptr);
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("player", "player_opening_demo_finished",
                                               "animation=Wait;control=enabled;forced_matrix=cleared");
        }
#endif
    }

    void incPlayerOxygen(u32 amount) {
        static_cast< void >(amount);
        throw std::logic_error("Player oxygen state is unavailable.");
    }
}  // namespace MR
