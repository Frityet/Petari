#include "compat/JkrAllocationDomain.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JAudio2/JAISound.hpp"

#include <aurora/exception.hpp>
#include <aurora/nw4r/brlan.hpp>
#include <aurora/sysconf.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <typeinfo>

namespace {
    using namespace smgpc::compat;

    void require(bool condition, const char* message) {
        if (!condition) {
            std::fprintf(stderr, "FAIL: %s\n", message);
            std::abort();
        }
    }

    void check_routing(const std::shared_ptr<JkrAllocationDomain>& domain, bool guest, bool callback_guest) {
        require(aurora::allocation::routing_state.guest == guest &&
                    aurora::allocation::routing_state.callbackGuest == callback_guest,
                "exception unwinding restores both routing flags");
        require(current_jkr_allocation_domain() == domain, "exception unwinding preserves the selected original heap");
        auto* original = new unsigned[8];
        require(JKRHeap::findFromRoot(original) == (guest ? &domain->heap() : nullptr),
                "subsequent original allocations use the restored routing");
        delete[] original;
    }

    template<class Exception>
    void check_exception(const std::shared_ptr<JkrAllocationDomain>& domain) {
        const auto previous = aurora::allocation::routing_state;
        const auto free_before = domain->heap().getFreeSize();
        bool caught = false;
        try {
            // This view deliberately has no terminator at its end.
            constexpr char input[] = "message substring: the ignored suffix";
            aurora::throw_host_exception<Exception>(std::string_view(input, 17));
        } catch (const Exception& error) {
            caught = true;
            require(typeid(error) == typeid(Exception), "the exact standard exception type survives");
            require(std::strcmp(error.what(), "message substring") == 0, "message views are copied with their exact length");
            require(JKRHeap::findFromRoot(const_cast<char*>(error.what())) == nullptr,
                    "retained exception message storage belongs to the host");
            require(domain->heap().getFreeSize() == free_before, "constructing native exceptions does not consume Game heap space");
        }
        require(caught, "the expected exception was caught");
        check_routing(domain, previous.guest, previous.callbackGuest);
    }

    void test_nested_routes(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        const auto free_before = runtime->root_heap().getFreeSize();
        {
            auto game = JkrAllocationDomain::create(runtime, 128U << 10);
            auto nested = JkrAllocationDomain::create(runtime, 128U << 10);
            JkrAllocationScope scope(game);
            check_exception<std::logic_error>(game);
            check_exception<std::domain_error>(game);
            check_exception<std::invalid_argument>(game);
            check_exception<std::length_error>(game);
            check_exception<std::out_of_range>(game);
            check_exception<std::runtime_error>(game);
            check_exception<std::range_error>(game);
            check_exception<std::overflow_error>(game);
            check_exception<std::underflow_error>(game);
            {
                aurora::allocation::HostAllocationScope host;
                check_exception<std::logic_error>(game);
                {
                    aurora::allocation::ClientAllocationScope callback;
                    check_exception<std::logic_error>(game);
                }
                check_routing(game, false, true);
                {
                    JkrAllocationScope nested_scope(nested);
                    aurora::allocation::HostAllocationScope nested_host;
                    aurora::allocation::ClientAllocationScope callback;
                    check_exception<std::logic_error>(nested);
                }
                check_routing(game, false, true);
            }
            check_routing(game, true, true);
        }
        check_routing(nullptr, false, false);
        require(runtime->root_heap().getFreeSize() == free_before, "nested scopes reclaim both complete scene arenas");
        std::puts("standard_exception_types=9 nested_routing=pass message_view=pass");
    }

    void test_retained_message(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        const auto free_before = runtime->root_heap().getFreeSize();
        std::exception_ptr retained;
        std::weak_ptr<JkrAllocationDomain> retired;
        {
            auto game = JkrAllocationDomain::create(runtime, 256U << 10);
            retired = game;
            JkrAllocationScope scope(game);
            // A caller's temporary may be Game-owned. Only the exception's
            // copied message is retained beyond this allocation scope.
            std::string message(4096, 'g');
            require(JKRHeap::findFromRoot(message.data()) == &game->heap(), "the caller message actually occupies the Game arena");
            try {
                aurora::throw_host_exception<std::logic_error>(message + ": complete");
            } catch (const std::logic_error& error) {
                std::logic_error copied(error);
                require(JKRHeap::findFromRoot(const_cast<char*>(copied.what())) == nullptr,
                        "copies retain the host-owned message");
                retained = std::current_exception();
                check_routing(game, true, true);
            }
        }
        require(retired.expired() && runtime->root_heap().getFreeSize() == free_before,
                "an exception pointer does not retain its retired scene arena");
        {
            auto replacement = JkrAllocationDomain::create(runtime, 256U << 10);
            JkrAllocationScope scope(replacement);
            auto* overwrite = new char[128U << 10];
            std::memset(overwrite, 0xcc, 128U << 10);
            try {
                std::rethrow_exception(retained);
            } catch (const std::logic_error& error) {
                const std::string_view message(error.what());
                require(typeid(error) == typeid(std::logic_error) && message.size() == 4106,
                        "rethrow preserves exact type and message after arena retirement");
                require(message.substr(4096) == ": complete" && message.substr(0, 4096).find_first_not_of('g') == std::string_view::npos,
                        "replacement scene allocations cannot overwrite the exception payload");
            }
            retained = {};
            delete[] overwrite;
        }
        require(runtime->root_heap().getFreeSize() == free_before, "replacement scene and exception storage are fully released");
        std::puts("guest_message_temporary=pass exception_copy=pass retained_rethrow_after_scene_retirement=pass");
    }

    void test_camera_unwind(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        const auto free_before = runtime->root_heap().getFreeSize();
        std::weak_ptr<JkrAllocationDomain> retired;
        bool caught = false;
        try {
            auto game = JkrAllocationDomain::create(runtime, 128U << 10);
            retired = game;
            JkrAllocationScope scope(game);
            (void)MR::getCamPos();
        } catch (const std::logic_error& error) {
            caught = true;
            require(retired.expired(), "the actual camera error is caught after its scene domain is destroyed");
            require(std::strcmp(error.what(), "Camera state is unavailable.") == 0,
                    "the real camera producer keeps its existing error semantics");
            require(JKRHeap::findFromRoot(const_cast<char*>(error.what())) == nullptr,
                    "the actual camera error retains a host-owned payload");
        }
        require(caught && runtime->root_heap().getFreeSize() == free_before, "camera error unwinding retires the Game heap and exception normally");
        std::puts("actual_camera_error_after_scene_retirement=pass");
    }

    void test_aurora_unwind(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        const auto free_before = runtime->root_heap().getFreeSize();
        const auto run = [&]<class Exception>(auto operation) {
            std::weak_ptr<JkrAllocationDomain> retired;
            bool caught = false;
            try {
                auto game = JkrAllocationDomain::create(runtime, 128U << 10);
                retired = game;
                JkrAllocationScope scope(game);
                operation();
            } catch (const Exception& error) {
                caught = true;
                require(typeid(error) == typeid(Exception) && retired.expired(),
                        "actual Aurora errors retain their exact type after arena retirement");
                require(error.what()[0] && JKRHeap::findFromRoot(const_cast<char*>(error.what())) == nullptr,
                        "actual Aurora API errors retain a nonempty host-owned message");
            }
            require(caught && runtime->root_heap().getFreeSize() == free_before,
                    "actual Aurora error callbacks fully reclaim the Game arena");
        };
        run.operator()<std::runtime_error>([] { (void)aurora::nw4r::lyt::parse_brlan_animation({}); });
        run.operator()<std::invalid_argument>([] { (void)aurora::SysConf::decode({}); });
        run.operator()<std::invalid_argument>([] { JAISoundHandle handle; handle.attachBackend(nullptr, 0); });
        std::puts("actual_aurora_brlan_sysconf_audio_handle_errors_after_scene_retirement=pass");
    }
}

int main() {
    const auto runtime = JkrHeapRuntime::create(4U << 20);
    test_nested_routes(runtime);
    test_retained_message(runtime);
    test_camera_unwind(runtime);
    test_aurora_unwind(runtime);
    return 0;
}
