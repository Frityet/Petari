#pragma once

#include <memory>
#include <string_view>
#include <revolution/types.h>

namespace nw4r::lyt {
class Pane;
class Group;
}

namespace smgpc::layout {
class LayoutRuntime;

// Stable SDK identities for the common BRLYT pane/group graph. Their resource
// kind remains explicit; a base record never claims a derived NW4R type.
class Nw4rLayoutRecords final {
public:
    explicit Nw4rLayoutRecords(LayoutRuntime& runtime);
    ~Nw4rLayoutRecords();
    Nw4rLayoutRecords(const Nw4rLayoutRecords&) = delete;
    Nw4rLayoutRecords& operator=(const Nw4rLayoutRecords&) = delete;

    [[nodiscard]] nw4r::lyt::Pane* pane(const char* name);
    [[nodiscard]] nw4r::lyt::Group* group(const char* name);
    [[nodiscard]] u32 group_index(const char* name) const;
    [[nodiscard]] u32 group_count() const;
    [[nodiscard]] u32 pane_count() const;
    [[nodiscard]] u32 pane_index(const nw4r::lyt::Pane* pane) const;
    void synchronize();
    void require_mutable_resource_graph(std::string_view operation) const;
    [[nodiscard]] u32 text_line_count(const char* pane_name) const;

private:
    struct State;
    std::unique_ptr<State> _state;
};

void synchronize_native_pane(const nw4r::lyt::Pane* pane);
void validate_native_pane_hierarchy_change(const nw4r::lyt::Pane* parent, const nw4r::lyt::Pane* child);
void validate_native_pane_rename(const nw4r::lyt::Pane* pane, const char* name);
void validate_native_group_append(const nw4r::lyt::Group* group);
void require_native_base_pane(const nw4r::lyt::Pane* pane, std::string_view operation);
} // namespace smgpc::layout
