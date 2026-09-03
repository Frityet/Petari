#include "runtime/ScenarioCatalogOwnership.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/StationedArchiveLoader.hpp"
#include "Game/Util/FileUtil.hpp"
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace smgpc::runtime {
namespace { ScenarioCatalogOwnership* active_catalog = nullptr; }

struct ScenarioCatalogOwnership::Storage {
    std::shared_ptr<compat::JkrAllocationDomain> domain;
    ArchiveMountService* mounts = nullptr;
    std::vector<std::shared_ptr<const MountedArchive>> archives;
    std::unique_ptr<ScenarioDataParser> parser;

    ~Storage() {
        parser.reset();
        if (mounts && domain) mounts->remove_for_heap(&domain->heap());
        // JMapInfo is an original JKRDisposer. Keep its source aliases and
        // typed storage alive until the domain performs original disposal.
        domain.reset();
        archives.clear();
    }
};

ScenarioCatalogOwnership::ScenarioCatalogOwnership(
    std::shared_ptr<compat::JkrHeapRuntime> runtime,
    std::size_t byte_budget, ArchiveMountService& mounts) {
    compat::JkrHostAllocationScope host;
    if (!runtime || ArchiveMountService::active() != &mounts)
        throw std::invalid_argument("The scenario catalog requires the active archive service and real heap runtime");
    if (active_catalog)
        throw std::logic_error("An actual scenario catalog is already published");

    // Validate the original fixed storage before the unchanged constructor
    // enumerates the same immutable disc directory. This does not select or
    // manufacture any catalog row.
    std::vector<std::string> paths;
    for (const auto& entry : mounts.dvd().directory_entries("/StageData")) {
        if (!entry.is_directory) continue;
        char path[256];
        MR::makeScenarioArchiveFileName(path, sizeof(path), entry.name.c_str());
        if (mounts.dvd().exists(path)) paths.emplace_back(path);
    }
    if (paths.size() > 64)
        throw std::length_error("Authored scenario archives exceed the original parser capacity");

    auto storage = std::make_unique<Storage>();
    storage->domain = compat::JkrAllocationDomain::create(std::move(runtime), byte_budget);
    storage->mounts = &mounts;
    StationedArchiveLoader::loadScenarioData(&storage->domain->heap());
    storage->archives.reserve(paths.size());
    for (const auto& path : paths) {
        auto archive = mounts.retain(path);
        if (!archive)
            throw std::logic_error("The original preloader did not publish an authored scenario archive");
        storage->archives.push_back(std::move(archive));
    }
    {
        compat::JkrAllocationScope heap(storage->domain);
        storage->parser = std::make_unique<ScenarioDataParser>("シナリオデータ解析");
        storage->parser->initWithoutIter();
    }
    compat::claim_name_obj_runtime_ownership(storage->parser.get(), this);
    _storage = std::move(storage);
    active_catalog = this;
}

ScenarioCatalogOwnership::~ScenarioCatalogOwnership() {
    compat::JkrHostAllocationScope host;
    if (active_catalog == this) active_catalog = nullptr;
    _storage.reset();
}
ScenarioDataParser& ScenarioCatalogOwnership::parser() const noexcept { return *_storage->parser; }
ScenarioCatalogOwnership* ScenarioCatalogOwnership::active() noexcept { return active_catalog; }
}
