#include "compat/JutTextureConstruction.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <aurora/exception.hpp>

#include <cassert>
#include <exception>
#include <map>
#include <mutex>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        struct Registry {
            std::mutex mutex;
            // Values point to stable list records; they identify a particular
            // construction, rather than just the reusable texture address.
            std::map<JUTTexture*, JUTTexture**> live;
        };
        Registry& registry() {
            static Registry value;
            return value;
        }
        thread_local JutTextureConstructionScope* current_scope;
    }

    JutTextureConstructionScope::JutTextureConstructionScope(bool enabled) noexcept
        : _previous(current_scope) {
        current_scope = enabled ? this : nullptr;
    }

    JutTextureConstructionScope::~JutTextureConstructionScope() {
        current_scope = _previous;
        _created.clear();
    }

    JutTextureOwnership::~JutTextureOwnership() { clear(); }

    void JutTextureOwnership::adopt(JutTextureConstructionScope& scope, JUTTexture* texture) noexcept {
        if (!texture) return;
        auto& created = scope._created._textures;
        for (auto it = created.begin(); it != created.end(); ++it) {
            if (it->texture == texture) {
                _textures.splice(_textures.end(), created, it);
                return;
            }
        }
    }

    void JutTextureOwnership::adopt_all(JutTextureConstructionScope& scope) noexcept {
        _textures.splice(_textures.end(), scope._created._textures);
    }

    void JutTextureOwnership::clear() noexcept {
        JkrHostAllocationScope host;
        auto& state = registry();
        while (!_textures.empty()) {
            auto& record = _textures.back();
            JUTTexture* texture = nullptr;
            {
                std::lock_guard lock(state.mutex);
                if (record.texture) {
                    const auto found = state.live.find(record.texture);
                    assert(found != state.live.end() && found->second == &record.texture);
                    if (found == state.live.end() || found->second != &record.texture) std::terminate();
                    texture = record.texture;
                    state.live.erase(found);
                    record.texture = nullptr;
                }
            }
            // Destruction can drain queued GPU commands; no registry lock is
            // held and a nested SDK destructor cannot destroy this record twice.
            delete texture;
            _textures.pop_back();
        }
    }

    void record_completed_jut_texture(JUTTexture& texture) {
        if (!current_scope) return;
        JkrHostAllocationScope host;
        auto& records = current_scope->_created._textures;
        records.push_back({&texture});
        try {
            auto& state = registry();
            std::lock_guard lock(state.mutex);
            if (!state.live.emplace(&texture, &records.back().texture).second)
                aurora::throw_host_exception<std::logic_error>("JUTTexture construction already has a live owner record");
        } catch (...) {
            records.pop_back();
            throw;
        }
    }

    void forget_completed_jut_texture(JUTTexture& texture) noexcept {
        JkrHostAllocationScope host;
        auto& state = registry();
        std::lock_guard lock(state.mutex);
        const auto found = state.live.find(&texture);
        if (found == state.live.end()) return;
        *found->second = nullptr;
        state.live.erase(found);
    }
}
