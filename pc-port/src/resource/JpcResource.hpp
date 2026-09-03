#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace smgpc::resource {
// Immutable, host-order backing for the original JPA data classes. Each block
// owns aligned storage containing the original typed header and its inline
// animation tables. Offsets remain relative to that header, as on Wii.
struct JpcBlock {
    std::uint32_t tag;
    std::size_t source_offset;
    std::span<const std::uint8_t> bytes;
    std::shared_ptr<const void> storage;
};
struct JpcResourceRecord {
    std::uint16_t user_index;
    std::uint8_t field_count;
    std::uint8_t key_count;
    std::uint8_t texture_reference_count;
    std::vector<JpcBlock> blocks;
};
class JpcResource final {
public:
    explicit JpcResource(std::span<const std::uint8_t>);
    [[nodiscard]] std::span<const std::uint8_t> source_bytes() const noexcept { return _source; }
    [[nodiscard]] const std::vector<JpcResourceRecord>& resources() const noexcept { return _resources; }
    [[nodiscard]] const std::vector<JpcBlock>& textures() const noexcept { return _textures; }
private:
    std::vector<std::uint8_t> _source;
    std::vector<JpcResourceRecord> _resources;
    std::vector<JpcBlock> _textures;
};
// Retains both the original archive identity and decoded backing until the
// unsized SDK loader takes its own lease. The manager retains that lease for
// its complete lifetime, including after this registration is released.
class JpcSourceRegistration final {
public:
    ~JpcSourceRegistration();
    JpcSourceRegistration(JpcSourceRegistration&&) noexcept;
    JpcSourceRegistration& operator=(JpcSourceRegistration&&) noexcept;
    JpcSourceRegistration(const JpcSourceRegistration&) = delete;
    JpcSourceRegistration& operator=(const JpcSourceRegistration&) = delete;
private:
    struct State;
    std::unique_ptr<State> _state;
    explicit JpcSourceRegistration(std::unique_ptr<State>);
    friend JpcSourceRegistration register_jpc_source(std::span<const std::uint8_t>, std::shared_ptr<const void>);
};
[[nodiscard]] JpcSourceRegistration register_jpc_source(std::span<const std::uint8_t>, std::shared_ptr<const void> source_owner);
[[nodiscard]] std::shared_ptr<const JpcResource> resolve_jpc_source(const void*);
} // namespace smgpc::resource
