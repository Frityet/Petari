#pragma once
#include "Game/Util/JMapInfo.hpp"
#include <cstdint>
#include <memory>
#include <span>
namespace smgpc::resource {
    // Explicitly retains one borrowed source identity for original unsized attach.
    // The caller retains the matching byte range until this registration ends.
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
    };
    // Copies share host-owned bytes, the decoded table and cached string lifetime.
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
    [[nodiscard]] std::shared_ptr<const JMapInfo> find_jmap_resource(const void *data);
}  // namespace smgpc::resource
