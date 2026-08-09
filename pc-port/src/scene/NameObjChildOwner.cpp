#include "scene/NameObjChildOwner.hpp"

#include <algorithm>
#include <stdexcept>

namespace smgpc::scene {

    NameObjChildOwner::~NameObjChildOwner() {
        clear();
    }

    void NameObjChildOwner::clear() noexcept {
        while (!_children.empty()) {
            _children.pop_back();
        }
    }

    bool NameObjChildOwner::empty() const noexcept {
        return _children.empty();
    }

    std::size_t NameObjChildOwner::size() const noexcept {
        return _children.size();
    }

    void NameObjChildOwner::validate_candidate(const NameObj *child) const {
        if (child == nullptr) {
            throw std::invalid_argument("A scene cannot own a null NameObj child.");
        }
        if (std::ranges::any_of(_children, [child](const auto &owned) {
                return owned.get() == child;
            })) {
            throw std::logic_error("A scene cannot adopt the same NameObj child twice.");
        }
    }

    void NameObjChildOwner::adopt_registered_since(
        smgpc::compat::NameObjRuntimeRegistrationMarker marker) {
        auto children =
            smgpc::compat::snapshot_name_obj_runtime_objects_since(marker);
        for (const auto *child : children) {
            validate_candidate(child);
        }

        _children.reserve(_children.size() + children.size());
        for (auto *child : children) {
            _children.emplace_back(child);
        }
    }

}  // namespace smgpc::scene
