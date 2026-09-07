#include "Game/Demo/DemoStartRequestHolder.hpp"
#include "Game/Demo/DemoStartRequestUtil.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoStartRequestOwner.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/NameObjChildOwner.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace {

    using smgpc::compat::DemoStartRequestOwner;

    static_assert(!std::is_copy_constructible_v<DemoStartRequestOwner>);
    static_assert(!std::is_move_constructible_v<DemoStartRequestOwner>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_0), LiveActor*>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_4), LayoutActor*>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_8), NerveExecutor*>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_C), NameObj*>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_10), NameObj*>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_14), DemoExecutor*>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_1C), const char*>);
    static_assert(std::is_same_v<decltype(DemoStartInfo::_20), const Nerve*>);

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_empty_record(const DemoStartInfo& info) {
        require(DemoStartRequestUtil::isEmpty(&info) && info._10 == nullptr &&
                    info._14 == nullptr && info.mDemoName == nullptr &&
                    info._1C == nullptr && info._20 == nullptr && info._24 == 0 &&
                    info._28 == 0 && info._2C == 0 && info._30 == 0 && info._34 == 0,
                "released original request slot retained borrowed data");
    }

    void test_typed_identity_and_copy() {
        DemoStartRequestOwner owner;
        auto& holder = owner.get();
        require(holder.mNumInfos == 16 && !holder.isExistRequest() &&
                    holder.getCurrentInfo() == nullptr,
                "new holder must have sixteen free records and no queued request");
        for (const auto* slot : holder.mStartInfos) {
            require_empty_record(*slot);
        }

        // The holder only compares these opaque identities, never dereferences
        // them. Reusing the same address across overloads checks that the four
        // original owner fields remain independent rather than being flattened.
        std::max_align_t identity{};
        auto* live = reinterpret_cast<LiveActor*>(&identity);
        auto* layout = reinterpret_cast<LayoutActor*>(&identity);
        auto* executor = reinterpret_cast<NerveExecutor*>(&identity);
        auto* object = reinterpret_cast<NameObj*>(&identity);
        auto* demo_executor = reinterpret_cast<DemoExecutor*>(&identity);
        auto* nerve = reinterpret_cast<const Nerve*>(&identity);
        constexpr auto name = "shared-name";
        constexpr auto part = "part-name";
        auto info = DemoStartInfo{};
        info._0 = live;
        info._10 = object;
        info._14 = demo_executor;
        info.mDemoName = name;
        info._1C = part;
        info._20 = nerve;
        info._24 = 3;
        info._28 = 1;
        info._2C = 1;
        info._30 = 2;
        info._34 = 1;
        holder.registerStartDemoInfo(info);
        info._0 = nullptr;
        info._4 = layout;
        holder.registerStartDemoInfo(info);
        info._4 = nullptr;
        info._8 = executor;
        holder.registerStartDemoInfo(info);
        info._8 = nullptr;
        info._C = object;
        holder.registerStartDemoInfo(info);

        const std::array found{holder.find(live, name), holder.find(layout, name),
                               holder.find(executor, name), holder.find(object, name)};
        for (std::size_t index = 0; index < found.size(); ++index) {
            require(found[index] == holder.mStartInfos[index],
                    "lookup lost its typed requester or first free slot identity");
            require(found[index]->_10 == object && found[index]->_14 == demo_executor &&
                        found[index]->_1C == part && found[index]->_20 == nerve &&
                        found[index]->_24 == 3 && found[index]->_28 == 1 &&
                        found[index]->_2C == 1 && found[index]->_30 == 2 && found[index]->_34 == 1,
                    "original assignment truncated pointers or dropped start options");
        }
        require(holder.find(live, "other-name") == nullptr,
                "lookup ignored the demo name");
        if constexpr (sizeof(std::uintptr_t) > sizeof(u32)) {
            const auto other_bits = reinterpret_cast<std::uintptr_t>(live) ^
                                    static_cast<std::uintptr_t>(std::uint64_t{1} << 32U);
            require(holder.find(reinterpret_cast<LiveActor*>(other_bits), name) == nullptr,
                    "lookup compared only the low 32 bits of the requester");
        }

        holder.pushRequest(live, name);
        holder.pushRequest(layout, name);
        holder.pushRequest(executor, name);
        holder.pushRequest(object, name);
        for (const auto* slot : found) {
            require(holder.getCurrentInfo() == slot, "typed requests lost FIFO order");
            holder.popRequest();
            require_empty_record(*slot);
        }
        require(!holder.isExistRequest() && holder.getCurrentInfo() == nullptr,
                "consumed requests remain active");
    }

    void test_ring_wrap_and_capacity() {
        DemoStartRequestOwner owner;
        auto& holder = owner.get();
        std::array<std::max_align_t, 64> identities{};
        auto requester = [&](std::size_t index) {
            return reinterpret_cast<NameObj*>(&identities[index]);
        };
        auto enqueue = [&](std::size_t index) {
            auto info = DemoStartInfo{};
            info._C = requester(index);
            info.mDemoName = "fifo";
            info._24 = static_cast<u32>(index);
            holder.registerStartDemoInfo(info);
            holder.pushRequest(info._C, info.mDemoName);
        };
        for (std::size_t index = 0; index < 16; ++index) {
            enqueue(index);
        }
        require(holder.findEmpty() == nullptr && holder.mRequestBuffer.mCount == 16,
                "sixteen live requests did not consume the original fixed capacity");

        // Retail push_back ignores a seventeenth queue entry. Use an existing
        // registered record so this does not exceed the separate slot precondition.
        holder.pushRequest(requester(0), "fifo");
        require(holder.mRequestBuffer.mCount == 16,
                "full queue must retain the original sixteen entries");
        for (std::size_t index = 0; index < identities.size(); ++index) {
            const auto* current = holder.getCurrentInfo();
            require(current != nullptr && current->_C == requester(index) &&
                        current->_24 == index,
                    "slot reuse changed FIFO order across ring wrap");
            holder.popRequest();
            require_empty_record(*current);
            if (index + 16 < identities.size()) {
                enqueue(index + 16);
            }
        }
        require(holder.getCurrentInfo() == nullptr && holder.findEmpty() != nullptr,
                "drained queue did not return its records for reuse");
    }

    void test_empty_slot_contract() {
        DemoStartRequestOwner owner;
        auto& holder = owner.get();
        auto* slot = holder.findEmpty();
        slot->_10 = holder.mProxyObj;
        slot->mDemoName = "metadata-only";
        slot->_24 = 3;
        require(DemoStartRequestUtil::isEmpty(slot),
                "only the four requester fields determine an empty slot");
        require(holder.findEmpty() == slot, "findEmpty changed the first-free ordering");
        require_empty_record(*slot);

        // This is an original low-level fallback, not a successful demo start.
        // Unregistered requests enqueue an empty record without inventing an owner.
        holder.pushRequest(holder.mProxyObj, "unregistered");
        require(holder.getCurrentInfo() == slot && DemoStartRequestUtil::isEmpty(slot),
                "unregistered request fabricated a start record");
        holder.popRequest();
        require(!holder.isExistRequest(), "empty fallback did not leave the queue");
    }

    void test_host_retirement_and_proxy_ownership() {
        using namespace smgpc::compat;
        const auto before = name_obj_runtime_state_count();
        auto heaps = JkrHeapRuntime::create(1U << 20);
        auto domain = JkrAllocationDomain::create(heaps, 64U << 10);
        auto owner = std::unique_ptr<DemoStartRequestOwner>{};
        NameObj borrower("borrowed request owner");
        const auto capture = mark_name_obj_runtime_registrations();
        NameObj* proxy = nullptr;
        {
            JkrAllocationScope game_allocations(domain);
            // The containing owner must also outlive the arena. Its internal
            // scope independently protects all original constructor allocations.
            {
                JkrHostAllocationScope host_owner;
                owner = std::make_unique<DemoStartRequestOwner>();
            }
            DemoStartRequestOwner nested;
            auto& holder = owner->get();
            proxy = holder.mProxyObj;
            require(JKRHeap::findFromRoot(&nested.get()) == nullptr &&
                        JKRHeap::findFromRoot(nested.get().mProxyObj) == nullptr,
                    "original holder or proxy used the active Game arena");
            for (auto* slot : nested.get().mStartInfos) {
                require(JKRHeap::findFromRoot(slot) == nullptr,
                        "original request storage used the active Game arena");
            }
            require(name_obj_runtime_owner(proxy) == owner.get(),
                    "construction capture can adopt the independently owned proxy");
            auto info = DemoStartInfo{};
            info._C = &borrower;
            info.mDemoName = "retained request";
            holder.registerStartDemoInfo(info);
            holder.pushRequest(&borrower, info.mDemoName);
        }
        smgpc::scene::NameObjChildOwner::rollback_registration_suffix(capture);
        require(has_name_obj_runtime_state(proxy),
                "construction capture deleted the independently owned proxy");
        domain.reset();
        {
            auto reuse = JkrAllocationDomain::create(heaps, 64U << 10);
            JkrAllocationScope game_allocations(reuse);
            auto* overwrite = new unsigned char[32U << 10];
            std::memset(overwrite, 0xa5, 32U << 10);
            delete[] overwrite;
        }
        require(owner->get().getCurrentInfo()->_C == &borrower &&
                    std::strcmp(owner->get().getCurrentInfo()->mDemoName, "retained request") == 0,
                "queued request did not survive unrelated Game arena retirement");
        owner.reset();
        require(!has_name_obj_runtime_state(proxy) &&
                    has_name_obj_runtime_state(&borrower) &&
                    name_obj_runtime_state_count() == before + 1,
                "owner retirement leaked its proxy or destroyed a borrowed requester");
    }

} // namespace

int main() {
    try {
        test_typed_identity_and_copy();
        test_ring_wrap_and_capacity();
        test_empty_slot_contract();
        test_host_retirement_and_proxy_ownership();
        std::cout << "[ok] original demo request holder: typed identities, FIFO, capacity, "
                     "slot reuse, and host ownership\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] original demo request holder: " << error.what() << '\n';
        return 1;
    }
}
