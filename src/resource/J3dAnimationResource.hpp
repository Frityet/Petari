#pragma once

#include "JSystem/J3DGraphLoader/J3DAnmLoader.hpp"

#include <cstdint>
#include <memory>
#include <span>

namespace smgpc::resource {

    class J3dAnimationSourceRegistration final {
    public:
        ~J3dAnimationSourceRegistration();
        J3dAnimationSourceRegistration(J3dAnimationSourceRegistration &&) noexcept;
        J3dAnimationSourceRegistration &operator=(J3dAnimationSourceRegistration &&) noexcept;
        J3dAnimationSourceRegistration(const J3dAnimationSourceRegistration &) = delete;
        J3dAnimationSourceRegistration &operator=(const J3dAnimationSourceRegistration &) = delete;

    private:
        struct State;
        std::unique_ptr<State> _state;
        explicit J3dAnimationSourceRegistration(std::unique_ptr<State>);
        friend class J3dAnimationResource;
    };

    // Retains bounded source bytes and every actual animation loaded from them.
    // Original SDK load returns borrowed pointers. Release them before releasing
    // the last resource owner; each load has independent mutable native tables.
    class J3dAnimationResource final {
    public:
        explicit J3dAnimationResource(std::span<const std::uint8_t> bytes);
        ~J3dAnimationResource();
        J3dAnimationResource(const J3dAnimationResource &) noexcept = default;
        J3dAnimationResource &operator=(const J3dAnimationResource &) noexcept = default;
        J3dAnimationResource(J3dAnimationResource &&) noexcept = default;
        J3dAnimationResource &operator=(J3dAnimationResource &&) noexcept = default;

        [[nodiscard]] const void *data() const noexcept;
        [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;
        // Register the real archive identity explicitly. The alias must remain
        // at this address for the registration lifetime. Its complete bytes
        // must equal the owned source; registration retains this resource.
        [[nodiscard]] J3dAnimationSourceRegistration register_source(std::span<const std::uint8_t> alias);
        [[nodiscard]] J3DAnmBase *load(J3DAnmLoaderDataBaseFlag flag = J3DLOADER_UNK_FLAG0);

    private:
        struct Storage;
        std::shared_ptr<Storage> _storage;
        friend J3DAnmBase *load_registered_j3d_animation(const void *, J3DAnmLoaderDataBaseFlag);
    };

    // Shared original-width file/native-pointer boundary. An unregistered
    // non-null resource fails before attempting an unsized host-memory read.
    [[nodiscard]] J3DAnmBase *load_registered_j3d_animation(const void *, J3DAnmLoaderDataBaseFlag);

    namespace detail {
        [[nodiscard]] J3DAnmBase *load_native_animation(const void *, J3DAnmLoaderDataBaseFlag);
        [[nodiscard]] const JUTDataBlockHeader *first_animation_block(const void *);
        [[nodiscard]] const JUTDataBlockHeader *next_animation_block(const void *, const JUTDataBlockHeader *);
    }  // namespace detail
}  // namespace smgpc::resource
