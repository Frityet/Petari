#pragma once

#include <memory>

class ModelManager;
namespace smgpc::compat {
    class JkrAllocationDomain;
    class ResourceHolderService;
    class ModelManagerOwner final {
    public:
        ModelManagerOwner(ResourceHolderService&, std::shared_ptr<JkrAllocationDomain>, const char*, const char*, bool);
        ~ModelManagerOwner();
        ModelManagerOwner(const ModelManagerOwner&) = delete;
        ModelManagerOwner& operator=(const ModelManagerOwner&) = delete;
        // Keep actual Game subgraphs/borrowed matrix storage through the model's
        // final packet use. Dependencies must not retain this owner (no cycle).
        void retain_lifetime_dependency(std::shared_ptr<void>);
        ModelManager& manager() const noexcept;
        const std::shared_ptr<JkrAllocationDomain>& allocation_domain() const noexcept;
    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
