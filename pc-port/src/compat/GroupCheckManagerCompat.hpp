#pragma once

#include <cstddef>

class GroupCheckManager;
class GroupChecker;
class NameObj;

namespace smgpc::compat {
    void register_group_checker(GroupChecker *checker);
    void release_group_checker(const GroupChecker *checker) noexcept;
    void add_group_checker_member(GroupChecker *checker, const NameObj *object);
    [[nodiscard]] bool group_checker_contains(const GroupChecker *checker, const NameObj *object);

    void initialize_group_check_manager(GroupCheckManager *manager);
    void release_group_check_manager(const GroupCheckManager *manager) noexcept;
    void add_group_check_manager_member(GroupCheckManager *manager, const NameObj *object, int group_index);
    [[nodiscard]] bool group_check_manager_contains(const GroupCheckManager *manager, const NameObj *object,
                                                    int group_index);

    [[nodiscard]] std::size_t group_checker_runtime_state_count() noexcept;
    [[nodiscard]] std::size_t group_check_manager_runtime_state_count() noexcept;
    [[nodiscard]] std::size_t attribute_group_membership_count() noexcept;
}  // namespace smgpc::compat
