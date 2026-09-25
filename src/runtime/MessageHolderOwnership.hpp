#pragma once

#include <cstddef>
#include <memory>
#include <string_view>

class MessageHolder;
class MessageData;

namespace smgpc::runtime {
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
    // Retire native BMG backing before the original archive and heap owners.
    void destroy_message_holder(MessageHolder*&) noexcept;
    [[nodiscard]] const char *message_id_for_pointer(const wchar_t *) noexcept;
}  // namespace smgpc::runtime
