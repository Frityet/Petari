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
        ModelManager& manager() const noexcept;
        const std::shared_ptr<JkrAllocationDomain>& allocation_domain() const noexcept;
    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
