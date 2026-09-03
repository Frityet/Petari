#include "OriginalMarioStateTests.hpp"

#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioState.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::tests {
    namespace {
        void require(bool condition, std::string_view message) {
            if (!condition) {
                throw std::runtime_error(std::string(message));
            }
        }

        class ScopedStatusStack final {
        public:
            explicit ScopedStatusStack(Mario& mario)
                : mario(mario), active(mario._97C), noticed(mario._980) {
                mario._97C = nullptr;
                mario._980 = nullptr;
            }

            ~ScopedStatusStack() {
                mario._97C = active;
                mario._980 = noticed;
            }

        private:
            Mario& mario;
            MarioState* active;
            MarioState* noticed;
        };

        struct StateEvent {
            u32 state;
            u32 message;
            u32 noticed = MarioStatus_None;

            bool operator==(const StateEvent&) const = default;
        };

        // Test-only message observers: these do not supply any production
        // MarioWall/Hang/Swim behavior. All stack/proc logic remains original.
        class RecordingState final : public MarioState {
        public:
            RecordingState(MarioActor& actor, u32 id, std::vector<StateEvent>& events)
                : MarioState(&actor, id), events(events) {
            }

            bool start() override {
                events.push_back({mStatusId, MarioStateMsg_Start});
                return start_result;
            }

            bool close() override {
                events.push_back({mStatusId, MarioStateMsg_Close});
                ++close_count;
                if (recursive_close) {
                    require(_10, "original proc must set its recursion flag before invoking close");
                    require(proc(MarioStateMsg_Close), "recursive close dispatch must return success");
                }
                return false; // Original proc(Close) deliberately ignores this result.
            }

            bool update() override {
                events.push_back({mStatusId, MarioStateMsg_Update});
                return update_result;
            }

            bool notice() override {
                events.push_back({mStatusId, MarioStateMsg_Notice, getNoticedStatus()});
                return notice_result;
            }

            bool keep() override {
                events.push_back({mStatusId, MarioStateMsg_Keep});
                return keep_result;
            }

            bool start_result = true;
            bool update_result = true;
            bool notice_result = true;
            bool keep_result = true;
            bool recursive_close = false;
            u32 close_count = 0;

        private:
            std::vector<StateEvent>& events;
        };

        void check_base_virtuals(MarioState& state) {
            const auto vector = TVec3f(1.0F, 2.0F, 3.0F);
            state.init();
            state.hitWall(vector, nullptr);
            state.hitPoly(3, vector, nullptr);
            state.draw3D();
            require(state.start() && state.close() && state.update() && state.keep(),
                    "retail base start/close/update/keep must return true");
            require(!state.notice() && !state.passRing(nullptr),
                    "retail base notice/passRing must return false");
            require(state.getBlurOffset() == 0.0F && !std::signbit(state.getBlurOffset()),
                    "retail base blur offset must be positive zero");
            require(state.proc(MarioStateMsg_Start) && state.proc(MarioStateMsg_Update) &&
                        state.proc(MarioStateMsg_Keep) && !state.proc(MarioStateMsg_Notice) &&
                        state.proc(MarioStateMsg_Close) && state.proc(99),
                    "original proc must dispatch known messages and accept an unknown message");
            require(!state._10, "close recursion flag must be cleared after base close");
        }
    }

    void verify_original_mario_state_lifecycle(MarioActor& actor) {
        require(actor.mMario != nullptr, "state lifecycle tests require the initialized real Mario owner");
        auto& mario = *actor.mMario;
        const auto restore = ScopedStatusStack(mario);

        {
            auto first = MarioState(&actor, MarioStatus_Wall);
            auto second = MarioState(&actor, MarioStatus_Hang);
            require(first.getPlayer() == &mario && first._8 == nullptr && !first._10,
                    "original state construction must retain the actual Mario owner and clear linkage");
            check_base_virtuals(first);
            mario.changeStatus(&first);
            require(mario.getCurrentStatus() == MarioStatus_Wall && mario.isStatusActive(MarioStatus_Wall),
                    "original changeStatus must install and expose the real state");
            mario.changeStatus(&second);
            require(mario._97C == &second && second._8 == nullptr && first._8 == nullptr &&
                        !mario.isStatusActive(MarioStatus_Wall) && mario._980 == &second,
                    "base notice=false must close the previous state before the next state starts");
            mario.closeStatus(nullptr);
            require(mario.getCurrentStatus() == MarioStatus_None,
                    "original closeStatus(nullptr) must empty the stack");
        }

        auto events = std::vector<StateEvent>{};
        {
            auto lower = RecordingState(actor, MarioStatus_Wall, events);
            auto upper = RecordingState(actor, MarioStatus_Hang, events);
            mario.changeStatus(&lower);
            mario.changeStatus(&upper);
            require(events == std::vector<StateEvent>{{MarioStatus_Wall, MarioStateMsg_Start},
                        {MarioStatus_Wall, MarioStateMsg_Notice, MarioStatus_Hang},
                        {MarioStatus_Hang, MarioStateMsg_Start}},
                    "new state identity must be visible to old states before its Start callback");
            require(mario._97C == &upper && upper._8 == &lower && mario.isStatusActive(MarioStatus_Wall),
                    "notice=true must retain the previous state below the new head");
            events.clear();
            mario.changeStatus(&lower);
            require(events.empty() && mario._97C == &upper && mario._980 == &upper,
                    "requesting an already active status must leave stack and notices unchanged");

            mario.sendStateMsg(MarioStateMsg_Update);
            require(events == std::vector<StateEvent>{{MarioStatus_Hang, MarioStateMsg_Update},
                        {MarioStatus_Wall, MarioStateMsg_Keep}},
                    "only the first successful state Update may run; retained states receive Keep");

            events.clear();
            upper.update_result = false;
            upper.recursive_close = true;
            mario.sendStateMsg(MarioStateMsg_Update);
            require(events == std::vector<StateEvent>{{MarioStatus_Hang, MarioStateMsg_Update},
                        {MarioStatus_Hang, MarioStateMsg_Close}, {MarioStatus_Wall, MarioStateMsg_Update}},
                    "a rejected head Update must close once and continue with Update on its saved successor");
            require(mario._97C == &lower && upper._8 == nullptr && upper.close_count == 1 && !upper._10,
                    "close dispatch must unlink, suppress recursion, and clear its original guard");
            mario.closeStatus(nullptr);
        }

        events.clear();
        {
            auto rejected = RecordingState(actor, MarioStatus_Hang, events);
            rejected.start_result = false;
            mario.changeStatus(&rejected);
            require(events == std::vector<StateEvent>{{MarioStatus_Hang, MarioStateMsg_Start},
                        {MarioStatus_Hang, MarioStateMsg_Close}} && mario._97C == nullptr && rejected._8 == nullptr,
                    "a rejected initial Start must receive Close and leave no active state");
        }

        events.clear();
        {
            auto lower = RecordingState(actor, MarioStatus_Wall, events);
            auto middle = RecordingState(actor, MarioStatus_Hang, events);
            auto upper = RecordingState(actor, MarioStatus_Swim, events);
            mario.changeStatus(&lower);
            mario.changeStatus(&middle);
            mario.changeStatus(&upper);
            events.clear();
            mario.closeStatus(&middle);
            require(mario._97C == &upper && upper._8 == &lower && middle._8 == nullptr &&
                        events == std::vector<StateEvent>{{MarioStatus_Hang, MarioStateMsg_Close}},
                    "closing a non-head state must splice the actual linked stack");
            events.clear();
            lower.keep_result = false;
            mario.sendStateMsg(MarioStateMsg_Update);
            require(events == std::vector<StateEvent>{{MarioStatus_Swim, MarioStateMsg_Update},
                        {MarioStatus_Wall, MarioStateMsg_Keep}, {MarioStatus_Wall, MarioStateMsg_Close}} &&
                        mario._97C == &upper && upper._8 == nullptr,
                    "a rejected Keep must remove that retained state without removing the successful head");
            mario.closeStatus(nullptr);
            require(mario._97C == nullptr && upper.close_count == 1,
                    "close-all must close each remaining state exactly once");
        }
    }
}
