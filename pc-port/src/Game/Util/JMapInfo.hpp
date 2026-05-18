#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <optional>
#include <string_view>
#include <type_traits>

#include <revolution.h>

class JMapInfo {
public:
    [[nodiscard]] bool dataExists() const {
        return false;
    }

    [[nodiscard]] int getNumEntries() const {
        return 0;
    }

    template < typename T >
    [[nodiscard]] bool getValue(int, const char*, T*) const {
        return false;
    }
};

class JMapInfoIter {
public:
    struct PlacementObject {
        const char* name = "";
        const char* type = "";
        s32 l_id = -1;
        std::array< s32, 8U > object_args{};
        std::array< f32, 3U > translation{};
        std::array< f32, 3U > rotation{};
        std::array< f32, 3U > scale{1.0F, 1.0F, 1.0F};
        bool has_translation = false;
        bool has_rotation = false;
        bool has_scale = false;
    };

    JMapInfoIter() = default;

    JMapInfoIter(const JMapInfo* pInfo, s32 index) : mInfo(pInfo), mIndex(index) {
    }

    explicit JMapInfoIter(const PlacementObject* pPlacement) : mPlacement(pPlacement) {
    }

    [[nodiscard]] bool isValid() const {
        return (mPlacement != nullptr) || (mInfo != nullptr && mIndex >= 0 && mIndex < mInfo->getNumEntries());
    }

    template < typename T >
    [[nodiscard]] bool getValue(const char* pKey, T* pValueOut) const {
        if (pKey == nullptr || pValueOut == nullptr) {
            return false;
        }

        if (mPlacement != nullptr && getPlacementValue(pKey, pValueOut)) {
            return true;
        }

        return mInfo != nullptr && mIndex >= 0 && mInfo->getValue(mIndex, pKey, pValueOut);
    }

    const JMapInfo* mInfo = nullptr;
    s32 mIndex = -1;

private:
    template < typename T >
    [[nodiscard]] bool getPlacementValue(const char* pKey, T* pValueOut) const {
        if constexpr (std::is_same_v< T, const char* >) {
            if (std::strcmp(pKey, "name") == 0) {
                *pValueOut = mPlacement->name != nullptr ? mPlacement->name : "";
                return true;
            }
            if (std::strcmp(pKey, "type") == 0 && mPlacement->type != nullptr && mPlacement->type[0] != '\0') {
                *pValueOut = mPlacement->type;
                return true;
            }
        } else if constexpr (std::is_same_v< T, s32 >) {
            if (std::strcmp(pKey, "l_id") == 0) {
                *pValueOut = mPlacement->l_id;
                return true;
            }

            constexpr auto prefix = std::string_view{"Obj_arg"};
            const auto key = std::string_view{pKey};
            if (key.starts_with(prefix) && key.size() == prefix.size() + 1U) {
                const auto arg_index_char = key.back();
                if (arg_index_char >= '0' && arg_index_char <= '7') {
                    *pValueOut = mPlacement->object_args[static_cast< std::size_t >(arg_index_char - '0')];
                    return true;
                }
            }
        } else if constexpr (std::is_same_v< T, bool >) {
            auto value = s32{};
            if (getPlacementValue(pKey, &value)) {
                *pValueOut = value != 0;
                return true;
            }
        } else if constexpr (std::is_same_v< T, f32 >) {
            auto value = s32{};
            if (getPlacementValue(pKey, &value)) {
                *pValueOut = static_cast< f32 >(value);
                return true;
            }
            const auto key = std::string_view{pKey};
            const auto component_index = [](char component) -> std::optional< std::size_t > {
                switch (component) {
                case 'x':
                    return 0U;
                case 'y':
                    return 1U;
                case 'z':
                    return 2U;
                default:
                    return std::nullopt;
                }
            };
            const auto read_vec_component = [&](std::string_view prefix, const std::array< f32, 3U>& values, bool has_values) {
                if (!has_values || key.size() != prefix.size() + 2U || key.substr(0U, prefix.size()) != prefix || key[prefix.size()] != '_') {
                    return false;
                }

                const auto index = component_index(key.back());
                if (!index.has_value()) {
                    return false;
                }

                *pValueOut = values[*index];
                return true;
            };

            if (read_vec_component("pos", mPlacement->translation, mPlacement->has_translation) ||
                read_vec_component("dir", mPlacement->rotation, mPlacement->has_rotation) ||
                read_vec_component("scale", mPlacement->scale, mPlacement->has_scale)) {
                return true;
            }
        }

        return false;
    }

    const PlacementObject* mPlacement = nullptr;
};
