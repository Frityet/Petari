#include <aurora/exception.hpp>
#include "compat/JutTextureAllocation.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/Mem1ResourceHeap.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"

#include <cassert>
#include <exception>
#include <map>
#include <mutex>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        struct Registry {
            std::mutex mutex;
            const JutTextureAllocationService* service = nullptr;
            std::shared_ptr<resource::Mem1ResourceHeap> heap;
            std::map<JUTTexture*, resource::Mem1ResourceHeap::Allocation> owned;
        };
        Registry& registry() {
            static Registry value;
            return value;
        }
    }

    JutTextureAllocationService::JutTextureAllocationService(std::shared_ptr<resource::Mem1ResourceHeap> heap)
        : _heap(std::move(heap)) {
        JkrHostAllocationScope host;
        if (!_heap) aurora::throw_host_exception<std::invalid_argument>("JUTTexture requires an explicit mapped graphics heap");
        auto& state = registry();
        std::lock_guard lock(state.mutex);
        if (state.service) aurora::throw_host_exception<std::logic_error>("A JUTTexture allocation service is already installed");
        state.service = this;
        state.heap = _heap;
    }

    JutTextureAllocationService::~JutTextureAllocationService() {
        JkrHostAllocationScope host;
        auto& state = registry();
        std::lock_guard lock(state.mutex);
        assert(state.service == this);
        state.service = nullptr;
        state.heap.reset();
    }

    JutTextureAllocation::JutTextureAllocation(JUTTexture* texture, void* data) noexcept : _texture(texture), _data(data) {}
    JutTextureAllocation::~JutTextureAllocation() {
        if (_texture) {
            GXDestroyTexObj(&_texture->mObj);
            release_owned_jut_texture(*_texture);
        }
    }

    JutTextureAllocation allocate_owned_jut_texture(JUTTexture& texture, std::size_t size) {
        JkrHostAllocationScope host;
        auto& state = registry();
        std::shared_ptr<resource::Mem1ResourceHeap> heap;
        {
            std::lock_guard lock(state.mutex);
            heap = state.heap;
        }
        if (!heap) aurora::throw_host_exception<std::logic_error>("Owned JUTTexture construction requires the process graphics heap");
        if (size < sizeof(ResTIMG)) aurora::throw_host_exception<std::length_error>("Owned JUTTexture storage must contain its complete header");
        auto allocation = heap->allocate(size);
        void* data = allocation.bytes().data();
        {
            std::lock_guard lock(state.mutex);
            if (state.owned.contains(&texture)) aurora::throw_host_exception<std::logic_error>("JUTTexture address still owns a mapped allocation");
            state.owned.emplace(&texture, std::move(allocation));
        }
        return JutTextureAllocation(&texture, data);
    }

    void release_owned_jut_texture(JUTTexture& texture) noexcept {
        JkrHostAllocationScope host;
        auto& state = registry();
        decltype(state.owned)::node_type node;
        {
            std::lock_guard lock(state.mutex);
            node = state.owned.extract(&texture);
        }
        assert(!node.empty());
        if (node.empty()) std::terminate();
        auto* image = node.mapped().bytes().data() + sizeof(ResTIMG);
        GXDestroyCopyTex(image);
        // The allocation drains queued GX CPU reads before this mapped range
        // becomes available for reuse. No registry lock is held during drain.
    }

    JUTTexture* get_owned_jut_texture(const ResTIMG* image) {
        JkrHostAllocationScope host;
        auto& state = registry();
        std::lock_guard lock(state.mutex);
        for (auto& [texture, allocation] : state.owned)
            if (allocation.bytes().data() == reinterpret_cast<const std::byte*>(image)) return texture;
        aurora::throw_host_exception<std::logic_error>("ResTIMG does not identify an owned JUTTexture allocation");
    }
}
