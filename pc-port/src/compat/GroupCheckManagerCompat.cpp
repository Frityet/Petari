#include "compat/GroupCheckManagerCompat.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {
    struct GroupCheckerRuntimeState {
        std::unordered_set<std::string> member_names{};
    };

    struct GroupCheckManagerRuntimeState {
        std::array<std::unique_ptr<GroupChecker>, 2> groups{};
    };

    [[nodiscard]] auto &group_checker_states() {
        static auto states = std::unordered_map<const GroupChecker *, GroupCheckerRuntimeState>{};
        return states;
    }

    [[nodiscard]] auto &group_check_manager_states() {
        static auto states = std::unordered_map<const GroupCheckManager *, GroupCheckManagerRuntimeState>{};
        return states;
    }

    [[nodiscard]] GroupCheckerRuntimeState &require_group_checker_state(const GroupChecker *checker) {
        if (checker == nullptr) {
            throw std::invalid_argument("Attribute group operation requires a GroupChecker.");
        }
        const auto found = group_checker_states().find(checker);
        if (found == group_checker_states().end()) {
            throw std::logic_error("GroupChecker has no registered native runtime state.");
        }
        return found->second;
    }

    [[nodiscard]] GroupCheckManagerRuntimeState &require_group_check_manager_state(
        const GroupCheckManager *manager) {
        if (manager == nullptr) {
            throw std::invalid_argument("Attribute group operation requires a GroupCheckManager.");
        }
        const auto found = group_check_manager_states().find(manager);
        if (found == group_check_manager_states().end()) {
            throw std::logic_error("GroupCheckManager has no registered native runtime state.");
        }
        return found->second;
    }

    [[nodiscard]] GroupChecker &require_group(GroupCheckManagerRuntimeState &state, int group_index) {
        if (group_index < 0 || group_index >= static_cast<int>(state.groups.size())) {
            throw std::out_of_range("Attribute group index must be in the retail 0..1 range.");
        }
        return *state.groups[static_cast<std::size_t>(group_index)];
    }

    [[nodiscard]] GroupCheckManager &require_active_group_check_manager() {
        auto *holder = MR::getSceneObjHolder();
        if (holder == nullptr) {
            throw std::logic_error("Attribute groups require an active scene object holder.");
        }

        auto *object = holder->getObj(SceneObj_GroupCheckManager);
        if (object == nullptr) {
            throw std::logic_error("The active scene has no pre-created GroupCheckManager.");
        }
        return *static_cast<GroupCheckManager *>(object);
    }

    [[nodiscard]] const NameObj &require_attribute_group_object(const LiveActor *actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("Attribute group membership requires a LiveActor.");
        }
        return *actor;
    }

    [[nodiscard]] const char *require_attribute_group_name(const NameObj *object) {
        if (object == nullptr) {
            throw std::invalid_argument("Attribute group membership requires a NameObj.");
        }
        if (object->mName == nullptr) {
            throw std::logic_error("Attribute group membership requires a registered NameObj name.");
        }
        return object->mName;
    }
}  // namespace

namespace smgpc::compat {
    void register_group_checker(GroupChecker *checker) {
        if (checker == nullptr) {
            throw std::invalid_argument("GroupChecker runtime state requires a real checker.");
        }
        if (!group_checker_states().try_emplace(checker).second) {
            throw std::logic_error("GroupChecker runtime state is already registered.");
        }
    }

    void release_group_checker(const GroupChecker *checker) noexcept {
        group_checker_states().erase(checker);
    }

    void add_group_checker_member(GroupChecker *checker, const NameObj *object) {
        require_group_checker_state(checker).member_names.emplace(require_attribute_group_name(object));
    }

    bool group_checker_contains(const GroupChecker *checker, const NameObj *object) {
        if (object == nullptr) {
            return false;
        }
        return require_group_checker_state(checker).member_names.contains(require_attribute_group_name(object));
    }

    void initialize_group_check_manager(GroupCheckManager *manager) {
        if (manager == nullptr) {
            throw std::invalid_argument("GroupCheckManager runtime state requires a real manager.");
        }
        if (group_check_manager_states().contains(manager)) {
            throw std::logic_error("GroupCheckManager runtime state is already registered.");
        }

        auto state = GroupCheckManagerRuntimeState{};
        state.groups[0] = std::make_unique<GroupChecker>("カメサーチ対象物グループ", 0x20U);
        smgpc::compat::claim_name_obj_runtime_ownership(
            state.groups[0].get(), manager);
        state.groups[1] = std::make_unique<GroupChecker>("スピニングボックス反射グループ", 0x8U);
        smgpc::compat::claim_name_obj_runtime_ownership(
            state.groups[1].get(), manager);
        auto *shell_search_group = state.groups[0].get();
        auto *spinning_box_search_group = state.groups[1].get();

        const auto [found, inserted] = group_check_manager_states().try_emplace(manager, std::move(state));
        if (!inserted) {
            throw std::logic_error("GroupCheckManager runtime state is already registered.");
        }
        (void)found;
        manager->mShellSearchGroup = shell_search_group;
        manager->mSpinningBoxSearchGroup = spinning_box_search_group;
    }

    void release_group_check_manager(const GroupCheckManager *manager) noexcept {
        group_check_manager_states().erase(manager);
    }

    void add_group_check_manager_member(GroupCheckManager *manager, const NameObj *object, int group_index) {
        auto &state = require_group_check_manager_state(manager);
        require_group(state, group_index).add(object);
    }

    bool group_check_manager_contains(const GroupCheckManager *manager, const NameObj *object, int group_index) {
        auto &state = require_group_check_manager_state(manager);
        return group_checker_contains(&require_group(state, group_index), object);
    }

    std::size_t group_checker_runtime_state_count() noexcept {
        return group_checker_states().size();
    }

    std::size_t group_check_manager_runtime_state_count() noexcept {
        return group_check_manager_states().size();
    }

    std::size_t attribute_group_membership_count() noexcept {
        auto count = std::size_t{};
        for (const auto &[checker, state] : group_checker_states()) {
            (void)checker;
            count += state.member_names.size();
        }
        return count;
    }
}  // namespace smgpc::compat

GroupChecker::GroupChecker(const char *name, u32) : NameObj(name), mHashTable(nullptr) {
    smgpc::compat::register_group_checker(this);
}

GroupChecker::~GroupChecker() {
    smgpc::compat::release_group_checker(this);
}

void GroupChecker::initAfterPlacement() {
    // The native HashSortTable sorts after placement. std::unordered_set keeps
    // the same name-keyed lookup contract without a separate sort phase.
}

void GroupChecker::add(const NameObj *object) {
    smgpc::compat::add_group_checker_member(this, object);
}

GroupCheckManager::GroupCheckManager(const char *name)
    : NameObj(name), mShellSearchGroup(nullptr), mSpinningBoxSearchGroup(nullptr), _14(2U) {
    smgpc::compat::initialize_group_check_manager(this);
}

GroupCheckManager::~GroupCheckManager() {
    smgpc::compat::release_group_check_manager(this);
    mShellSearchGroup = nullptr;
    mSpinningBoxSearchGroup = nullptr;
}

void GroupCheckManager::add(const NameObj *object, s32 group_index) {
    smgpc::compat::add_group_check_manager_member(this, object, group_index);
}

bool GroupCheckManager::isExist(const NameObj *object, s32 group_index) {
    return smgpc::compat::group_check_manager_contains(this, object, group_index);
}

namespace MR {
    void addToAttributeGroupSearchTurtle(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        require_active_group_check_manager().add(&object, 0);
    }

    void addToAttributeGroupReflectSpinningBox(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        require_active_group_check_manager().add(&object, 1);
    }

    bool isExistInAttributeGroupSearchTurtle(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        return require_active_group_check_manager().isExist(&object, 0);
    }

    bool isExistInAttributeGroupReflectSpinningBox(const LiveActor *actor) {
        const auto &object = require_attribute_group_object(actor);
        return require_active_group_check_manager().isExist(&object, 1);
    }
}  // namespace MR
