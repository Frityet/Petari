#include "runtime/ParticleResourceOwnership.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <stdexcept>
#include <utility>

namespace smgpc::runtime {
namespace {
    ParticleResourceOwnership* active_particles = nullptr;
    constexpr const char* particle_archive = "/ParticleData/Effect.arc";
}

struct ParticleResourceOwnership::Storage {
    std::shared_ptr<compat::JkrAllocationDomain> domain;
    ArchiveMountService* mounts = nullptr;
    std::shared_ptr<const MountedArchive> archive;
    std::unique_ptr<ParticleResourceHolder> holder;
    std::size_t construction_bytes = 0;

    ~Storage() {
        holder.reset();
        if (mounts && domain) mounts->remove_for_heap(&domain->heap());
        // Original JMap disposers run before JPA's heap finalizer. Both may
        // release native backing, so the mounted archive outlives the domain.
        domain.reset();
        archive.reset();
    }
};

ParticleResourceOwnership::ParticleResourceOwnership(
    std::shared_ptr<compat::JkrHeapRuntime> runtime,
    std::size_t byte_budget, ArchiveMountService& mounts) {
    compat::JkrHostAllocationScope host;
    if (!runtime || ArchiveMountService::active() != &mounts)
        throw std::invalid_argument("Particle resources require the active archive service and real heap runtime");
    if (active_particles)
        throw std::logic_error("An actual particle resource holder is already published");

    auto storage = std::make_unique<Storage>();
    storage->domain = compat::JkrAllocationDomain::create(std::move(runtime), byte_budget);
    storage->mounts = &mounts;
    // Load before entering the unchanged constructor. This propagates archive
    // errors, while the original constructor remounts the same ready identity.
    auto* archive = mounts.mount(particle_archive, &storage->domain->heap());
    storage->archive = mounts.retain(particle_archive);
    if (!storage->archive || &storage->archive->archive() != archive)
        throw std::logic_error("Particle resource preload lost its actual mounted archive");
    for (const auto* resource : {"Particles.jpc", "ParticleNames.bcsv", "AutoEffectList.bcsv"})
        if (!archive->getResource(resource))
            throw std::runtime_error("Particle resource archive is missing an original required resource");
    const auto initial_free = storage->domain->heap().getFreeSize();
    {
        compat::JkrAllocationScope heap(storage->domain);
        storage->holder = std::make_unique<ParticleResourceHolder>(particle_archive);
    }
    storage->construction_bytes = initial_free - storage->domain->heap().getFreeSize();
    _storage = std::move(storage);
    active_particles = this;
}

ParticleResourceOwnership::~ParticleResourceOwnership() {
    compat::JkrHostAllocationScope host;
    if (active_particles == this) active_particles = nullptr;
    _storage.reset();
}
ParticleResourceHolder& ParticleResourceOwnership::holder() const noexcept { return *_storage->holder; }
std::size_t ParticleResourceOwnership::construction_bytes() const noexcept { return _storage->construction_bytes; }
ParticleResourceOwnership* ParticleResourceOwnership::active() noexcept { return active_particles; }
}
