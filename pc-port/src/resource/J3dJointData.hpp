#pragma once

#include <cstdint>
#include <memory>
#include <span>

class J3DModelData;

namespace smgpc::resource {

    // Retained native data for the INF1/JNT1/EVP1/DRW1 portion of a model.
    // This is a construction component, not a complete model loader. The full
    // resource owner must retain it, build the other model tables, validate their
    // hierarchy references, and perform the original hierarchy/finalization.
    class J3dJointData final {
    public:
        explicit J3dJointData(std::span<const std::uint8_t> model_bytes,
                              std::uint32_t load_flags = 0);
        ~J3dJointData();
        J3dJointData(J3dJointData&&) noexcept;
        J3dJointData& operator=(J3dJointData&&) = delete;
        J3dJointData(const J3dJointData&) = delete;
        J3dJointData& operator=(const J3dJointData&) = delete;

        // Attach once to a model whose joint data has not yet been populated.
        // Owns no ModelData and does not set mpRawData, link a hierarchy, or
        // calculate matrices. All attached pointers live until this owner dies.
        void attach_to(J3DModelData& model_data);

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

}  // namespace smgpc::resource
