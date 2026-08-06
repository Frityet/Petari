#include "Game/Util/PlayerUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoUtilCompat.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>
#include <cstddef>
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
        constexpr f32 cRadToDeg = 180.0F / 3.14159265358979323846F;

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
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            player->show_player();
        }
    }

    void hidePlayer() {
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            player->hide_player();
        }
    }

    TVec3f* getPlayerPos() {
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            if (auto *actor = player->attached_actor()) {
                return &actor->mPosition;
            }
            static TVec3f position{};
            const auto values = player->position();
            position.set(values[0U], values[1U], values[2U]);
            return &position;
        }
        static TVec3f position{};
        position.zero();
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
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            if (auto *actor = player->attached_actor()) {
                return &actor->mRotation;
            }
        }
        static TVec3f rotation{};
        TVec3f front{};
        getPlayerFrontVec(&front);
        rotation.set(0.0F, std::atan2(front.x, front.z) * cRadToDeg, 0.0F);
        return &rotation;
    }

    TVec3f* getPlayerVelocity() {
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            if (auto *actor = player->attached_actor()) {
                return &actor->mVelocity;
            }
            static TVec3f velocity{};
            const auto values = player->velocity();
            velocity.set(values[0U], values[1U], values[2U]);
            return &velocity;
        }
        static TVec3f velocity{};
        velocity.zero();
        return &velocity;
    }

    TVec3f* getPlayerGravity() {
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            if (auto *actor = player->attached_actor()) {
                return &actor->mGravity;
            }
            static TVec3f gravity{};
            const auto values = player->gravity();
            gravity.set(values[0U], values[1U], values[2U]);
            return &gravity;
        }
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
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            player->set_base_matrix(matrix);
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

        if (const auto *player = smgpc::compat::active_player_system_for_player_util();
            player != nullptr && player->has_base_matrix()) {
            const auto values = player->base_matrix();
            auto index = std::size_t{};
            for (auto row = 0U; row < 3U; ++row) {
                for (auto column = 0U; column < 4U; ++column) {
                    matrix[row][column] = values[index++];
                }
            }
        }

        return matrix;
    }

    void startBckPlayer(const char* pName, const char* pFileName) {
        auto *player = smgpc::compat::active_player_system_for_player_util();
        auto *actor = player != nullptr ? player->attached_actor() : nullptr;
        if (actor == nullptr) {
            return;
        }
        actor->startBck(pName, pFileName);
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            const auto *model = smgpc::compat::actor_model(actor);
            const auto frame_max = model != nullptr && pName != nullptr ? model->bck_frame_max(pName) : std::nullopt;
            runtime->emit_semantic_trace_event(
                "player", "player_bck_started",
                "name=" + std::string(pName != nullptr ? pName : "") +
                    ";file=" + std::string(pFileName != nullptr ? pFileName : "") +
                    ";frame_max=" + std::to_string(frame_max.value_or(0)));
        }
#endif
    }

    s16 getBckFrameMaxPlayer(const char* pName) {
        const auto *player = smgpc::compat::active_player_system_for_player_util();
        const auto *actor = player != nullptr ? player->attached_actor() : nullptr;
        const auto *model = smgpc::compat::actor_model(actor);
        return model != nullptr && pName != nullptr ? model->bck_frame_max(pName).value_or(0) : 0;
    }

    bool isBckStoppedPlayer() {
        const auto *player = smgpc::compat::active_player_system_for_player_util();
        const auto *actor = player != nullptr ? player->attached_actor() : nullptr;
        const auto *model = smgpc::compat::actor_model(actor);
        const auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        return model == nullptr || runtime == nullptr || model->is_bck_stopped(runtime->frame_index());
    }

    void initPlayerAfterOpeningDemo() {
        smgpc::compat::release_puppetable_demo_control(true);
        auto *player = smgpc::compat::active_player_system_for_player_util();
        auto *actor = player != nullptr ? player->attached_actor() : nullptr;
        if (player != nullptr) {
            player->finish_opening_demo();
        }
        if (actor != nullptr) {
            actor->startBck("Wait", nullptr);
        }
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("player", "player_opening_demo_finished",
                                               "animation=Wait;control=enabled;forced_matrix=cleared");
        }
#endif
    }

    void incPlayerOxygen(u32 amount) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("player", "oxygen_increased", "amount=" + std::to_string(amount));
        }
#else
        static_cast< void >(amount);
#endif
    }
}  // namespace MR
