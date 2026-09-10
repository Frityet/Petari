#pragma once

#include <cstddef>
#include <memory>
#include <string_view>

class MessageHolder;
class MessageData;
namespace smgpc::compat {
    class JkrHeapRuntime;
}

namespace smgpc::runtime {
    class ArchiveMountService;

    // Owns the complete original system/game message records and their archive
    // aliases. Publication is scoped until GameSystem owns this same class.
    class MessageHolderOwnership final {
    public:
        MessageHolderOwnership(std::shared_ptr<compat::JkrHeapRuntime>, std::size_t byte_budget,
                               ArchiveMountService &, std::string_view game_archive_path,
                               std::string_view language_prefix);
        ~MessageHolderOwnership();
        MessageHolderOwnership(const MessageHolderOwnership &) = delete;
        MessageHolderOwnership &operator=(const MessageHolderOwnership &) = delete;
        [[nodiscard]] MessageHolder &holder() const noexcept;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    // The retail scene aliases the persistent game messages. Keep that alias
    // valid across scene-owned object construction and retirement.
    class SceneMessageBinding final {
    public:
        explicit SceneMessageBinding(MessageHolder &holder);
        ~SceneMessageBinding();
        SceneMessageBinding(const SceneMessageBinding &) = delete;
        SceneMessageBinding &operator=(const SceneMessageBinding &) = delete;

    private:
        MessageHolder &_holder;
        MessageData *_previous;
    };

    [[nodiscard]] MessageHolder *current_message_holder() noexcept;
    [[nodiscard]] MessageHolder &require_message_holder();
    [[nodiscard]] const char *message_id_for_pointer(const wchar_t *) noexcept;
}  // namespace smgpc::runtime
