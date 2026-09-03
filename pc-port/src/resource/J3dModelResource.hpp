#pragma once

#include <cstdint>
#include <memory>
#include <span>

class J3DModelData;
class J3DMaterialTable;

namespace smgpc::compat { class JkrAllocationDomain; }

namespace smgpc::resource {
    class Mem1ResourceHeap;

    class J3dModelSourceRegistration final {
    public:
        ~J3dModelSourceRegistration();
        J3dModelSourceRegistration(J3dModelSourceRegistration&&) noexcept;
        J3dModelSourceRegistration& operator=(J3dModelSourceRegistration&&) noexcept;
        J3dModelSourceRegistration(const J3dModelSourceRegistration&) = delete;
        J3dModelSourceRegistration& operator=(const J3dModelSourceRegistration&) = delete;

    private:
        struct State;
        std::unique_ptr<State> _state;
        explicit J3dModelSourceRegistration(std::unique_ptr<State>);
        friend class J3dModelResource;
    };

    // Retains authored bytes, actual loaded model/table instances, native
    // metadata and their original allocation domain. SDK returns are borrowed
    // for the lifetime of the retained resource or an explicit source alias.
    class J3dModelResource final {
    public:
        J3dModelResource(std::span<const std::uint8_t>,
                         std::shared_ptr<compat::JkrAllocationDomain>,
                         std::shared_ptr<Mem1ResourceHeap>);
        ~J3dModelResource();
        J3dModelResource(const J3dModelResource&) noexcept = default;
        J3dModelResource& operator=(const J3dModelResource&) noexcept = default;
        J3dModelResource(J3dModelResource&&) noexcept = default;
        J3dModelResource& operator=(J3dModelResource&&) noexcept = default;

        [[nodiscard]] const void* data() const noexcept;
        [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;
        [[nodiscard]] J3dModelSourceRegistration register_source(std::span<const std::uint8_t>);
        [[nodiscard]] J3DModelData* load(std::uint32_t flags);
        [[nodiscard]] J3DMaterialTable* load_material_table();

    private:
        struct Storage;
        std::shared_ptr<Storage> _storage;
        friend J3DModelData* load_registered_j3d_model(const void*, std::uint32_t, bool);
        friend J3DMaterialTable* load_registered_j3d_material_table(const void*);
    };

    [[nodiscard]] J3DModelData* load_registered_j3d_model(const void*, std::uint32_t flags, bool binary_display_list);
    [[nodiscard]] J3DMaterialTable* load_registered_j3d_material_table(const void*);
}
