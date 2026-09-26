#pragma once
#include "Game/Util/JMapInfo.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
namespace smgpc::resource {
    // Explicitly retains one borrowed source identity for original unsized attach.
    // The caller retains borrowed alias bytes through their use by attached readers.
    // Archive registrations instead retain their explicit source owner.
    class JMapSourceRegistration final {
    public:
        ~JMapSourceRegistration();
        JMapSourceRegistration(JMapSourceRegistration &&) noexcept;
        JMapSourceRegistration &operator=(JMapSourceRegistration &&) noexcept;
        JMapSourceRegistration(const JMapSourceRegistration &) = delete;
        JMapSourceRegistration &operator=(const JMapSourceRegistration &) = delete;

    private:
        struct State;
        std::unique_ptr<State> _state;
        explicit JMapSourceRegistration(std::unique_ptr<State>);
        friend class JMapResource;
        friend JMapSourceRegistration register_jmap_source(std::span<const std::uint8_t>, std::shared_ptr<const void>,
                                                          std::function<std::shared_ptr<const void>()>);
    };
    // Publish the complete bounds of retained immutable archive bytes. Validate
    // only if original Game code later attaches this identity as a JMapInfo.
    // Non-table files can share this boundary without being parsed as tables.
    // Fixed buffers provide retain_attachment to acquire a separate archive
    // retirement lease for each attached reader, before accessing raw bytes.
    [[nodiscard]] JMapSourceRegistration register_jmap_source(
        std::span<const std::uint8_t> bytes, std::shared_ptr<const void> source_owner,
        std::function<std::shared_ptr<const void>()> retain_attachment = {});
    // Copies share the unchanged host-owned resource bytes.
    class JMapResource final {
    public:
        explicit JMapResource(std::span<const std::uint8_t> bytes);
        [[nodiscard]] const void *data() const;
        [[nodiscard]] std::span<const std::uint8_t> bytes() const;
        [[nodiscard]] JMapSourceRegistration register_source(std::span<const std::uint8_t>);

    private:
        struct Storage;
        std::shared_ptr<Storage> _storage;
    };
    [[nodiscard]] JMapInfo make_jmap_info(std::span<const std::uint8_t> bytes);
    [[nodiscard]] std::shared_ptr<const JMapInfo> find_jmap_resource(const void *data);
}  // namespace smgpc::resource
