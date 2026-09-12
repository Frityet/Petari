#include "compat/TalkDirectorLifetime.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NPC/TalkBalloon.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkState.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <aurora/exception.hpp>
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        void release_balloon(TalkBalloon* balloon, const TalkMessageCtrl* controller) noexcept {
            if (has_name_obj_runtime_state(balloon) && balloon->mMessageCtrl == controller)
                balloon->mMessageCtrl = nullptr;
        }

        bool has_active_balloon(const TalkDirector& director, const TalkMessageCtrl* controller) noexcept {
            const auto borrows = [&](const TalkBalloon* balloon) {
                return has_name_obj_runtime_state(balloon) && !balloon->mFlag.mIsDead &&
                       balloon->mMessageCtrl == controller;
            };
            const auto& balloons = *director.mBalloonHolder;
            for (s32 i = 0; i < 4; ++i)
                if (borrows(balloons.mBalloonShortArray[i])) return true;
            return borrows(balloons.mBalloonEvent) || borrows(balloons.mBalloonInfo) ||
                   borrows(balloons.mBalloonSign) || borrows(balloons.mBalloonIcon) ||
                   borrows(director.mStateHolder->mBalloonShort);
        }

        void release_controller(TalkDirector& director, const TalkMessageCtrl* controller) noexcept {
            for (auto** reference : {&director.mMsgCtrl, &director._3C,
                                     &director._40, &director._44}) {
                if (*reference == controller) *reference = nullptr;
            }

            auto& states = *director.mStateHolder;
            for (TalkState* state : {states.mTalk, static_cast<TalkState*>(states.mTalkShort),
                                    static_cast<TalkState*>(states.mTalkNormal),
                                    static_cast<TalkState*>(states.mTalkEvent),
                                    static_cast<TalkState*>(states.mTalkCompose)}) {
                if (state->_04 == controller) state->_04 = nullptr;
            }

            auto& balloons = *director.mBalloonHolder;
            for (s32 i = 0; i < 4; ++i) release_balloon(balloons.mBalloonShortArray[i], controller);
            release_balloon(balloons.mBalloonEvent, controller);
            release_balloon(balloons.mBalloonInfo, controller);
            release_balloon(balloons.mBalloonSign, controller);
            release_balloon(balloons.mBalloonIcon, controller);
            release_balloon(states.mBalloonShort, controller);
        }
    }

    void TalkDirectorLifetime::capture_after_init(TalkDirector& director) {
        if (_director || _retiring || !current_jkr_allocation_domain())
            aurora::throw_host_exception<std::logic_error>(
                "Original TalkDirector capture requires one completed owner in its scene Game heap");
        const auto generation = name_obj_runtime_generation(&director);
        if (!generation)
            aurora::throw_host_exception<std::logic_error>(
                "Original TalkDirector capture requires a registered NameObj");
        _director = &director;
        _generation = generation;
    }

    void TalkDirectorLifetime::begin_retirement() noexcept {
        _retiring = true;
    }

    void TalkDirectorLifetime::release_name_obj(const NameObj* object) noexcept {
        if (!_director || name_obj_runtime_generation(_director) != _generation) return;
        if (object == _director) {
            _director = nullptr;
            return;
        }

        auto& director = *_director;
        auto& controllers = director.mMsgControls;
        const auto remove = [&](TalkMessageCtrl* controller) {
            if (controller != object && controller->mHostActor != object) return false;
            // Retail actors die through kill(); their storage stays alive until
            // scene retirement. Deleting an active native participant cannot
            // manufacture the original camera/demo/nerve termination sequence.
            if (!_retiring && ((director.mTalkState && director.mTalkState->_04 == controller) ||
                               has_active_balloon(director, controller)))
                std::terminate();
            release_controller(director, controller);
            return true;
        };
        auto* old_end = controllers.end();
        auto* new_end = std::remove_if(controllers.begin(), old_end, remove);
        std::fill(new_end, old_end, nullptr);
        controllers.mCount = static_cast<s32>(new_end - controllers.begin());
    }

} // namespace smgpc::compat
