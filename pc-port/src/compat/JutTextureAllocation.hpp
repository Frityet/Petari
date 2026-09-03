#pragma once

#include <cstddef>
#include <memory>

class JUTTexture;
struct ResTIMG;
namespace smgpc::resource { class Mem1ResourceHeap; }

namespace smgpc::compat {
    // A process explicitly installs its mapped graphics heap. Allocated
    // texture storage keeps an independent lease after this service retires.
    class JutTextureAllocationService final {
    public:
        explicit JutTextureAllocationService(std::shared_ptr<resource::Mem1ResourceHeap>);
        ~JutTextureAllocationService();
        JutTextureAllocationService(const JutTextureAllocationService&) = delete;
        JutTextureAllocationService& operator=(const JutTextureAllocationService&) = delete;
    private:
        std::shared_ptr<resource::Mem1ResourceHeap> _heap;
    };

    class JutTextureAllocation final {
    public:
        ~JutTextureAllocation();
        JutTextureAllocation(const JutTextureAllocation&) = delete;
        JutTextureAllocation& operator=(const JutTextureAllocation&) = delete;
        [[nodiscard]] void* data() const noexcept { return _data; }
        void commit() noexcept { _texture = nullptr; }
    private:
        JutTextureAllocation(JUTTexture*, void*) noexcept;
        JUTTexture* _texture;
        void* _data;
        friend JutTextureAllocation allocate_owned_jut_texture(JUTTexture&, std::size_t);
    };

    [[nodiscard]] JutTextureAllocation allocate_owned_jut_texture(JUTTexture&, std::size_t);
    // Allows a native enclosing owner to adopt an actual texture constructed
    // by unchanged Game code, using its exact owned allocation identity.
    [[nodiscard]] JUTTexture* get_owned_jut_texture(const ResTIMG*);
    void release_owned_jut_texture(JUTTexture&) noexcept;
}
