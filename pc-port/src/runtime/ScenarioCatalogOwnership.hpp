#pragma once

#include <cstddef>
#include <memory>

class ScenarioDataParser;
namespace smgpc::compat { class JkrHeapRuntime; }

namespace smgpc::runtime {
    class ArchiveMountService;

    // Owns the actual process catalog and its original heap. Scene users
    // retain this owner; its publication never fabricates a GameSystem.
    class ScenarioCatalogOwnership final {
    public:
        ScenarioCatalogOwnership(std::shared_ptr<compat::JkrHeapRuntime>,
                                 std::size_t byte_budget,
                                 ArchiveMountService&);
        ~ScenarioCatalogOwnership();
        ScenarioCatalogOwnership(const ScenarioCatalogOwnership&) = delete;
        ScenarioCatalogOwnership& operator=(const ScenarioCatalogOwnership&) = delete;
        [[nodiscard]] ScenarioDataParser& parser() const noexcept;
        [[nodiscard]] static ScenarioCatalogOwnership* active() noexcept;
    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
