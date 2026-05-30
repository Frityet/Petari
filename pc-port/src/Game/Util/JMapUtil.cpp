#include "Game/Util/JMapUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace {
    [[nodiscard]] bool is_equal_string_case(const char* lhs, const char* rhs) {
        if (lhs == nullptr || rhs == nullptr) {
            return false;
        }

        return std::strcmp(lhs, rhs) == 0;
    }

    template < typename T >
    [[nodiscard]] bool get_arg_no_init(const JMapInfoIter& rIter, const char* pName, T* pOut) {
        if (pOut == nullptr) {
            return false;
        }

        auto value = s32{};
        if (!rIter.getValue(pName, &value) || value == -1) {
            return false;
        }

        if constexpr (std::is_same_v< T, bool >) {
            *pOut = value != 0;
        } else {
            *pOut = static_cast< T >(value);
        }
        return true;
    }

    template < typename T >
    [[nodiscard]] bool get_arg_and_init(const JMapInfoIter& rIter, const char* pName, T* pOut) {
        if (pOut == nullptr) {
            return false;
        }

        if constexpr (std::is_same_v< T, bool >) {
            *pOut = false;
        } else {
            *pOut = static_cast< T >(-1);
        }
        return get_arg_no_init(rIter, pName, pOut);
    }

    [[nodiscard]] std::array< char, 16U > obj_arg_name(std::size_t index) {
        auto name = std::array< char, 16U >{};
        std::snprintf(name.data(), name.size(), "Obj_arg%zu", index);
        return name;
    }

    template < typename T >
    [[nodiscard]] bool get_obj_arg_no_init(const JMapInfoIter& rIter, std::size_t index, T* pOut) {
        const auto name = obj_arg_name(index);
        return get_arg_no_init(rIter, name.data(), pOut);
    }

    template < typename T >
    [[nodiscard]] bool get_obj_arg_with_init(const JMapInfoIter& rIter, std::size_t index, T* pOut) {
        const auto name = obj_arg_name(index);
        return get_arg_and_init(rIter, name.data(), pOut);
    }

    [[nodiscard]] bool get_vec3_components(const JMapInfoIter& rIter, const char* x, const char* y, const char* z, TVec3f* pOut) {
        return pOut != nullptr && rIter.getValue(x, &pOut->x) && rIter.getValue(y, &pOut->y) && rIter.getValue(z, &pOut->z);
    }

    [[nodiscard]] s32 link_id(const JMapInfoIter& rIter) {
        auto id = s32{-1};
        (void)rIter.getValue("l_id", &id);
        return id;
    }

    [[nodiscard]] const JMapInfo* child_info(const JMapInfoIter& rIter) {
        return rIter.mInfo != nullptr ? rIter.mInfo->getChildObjInfo() : nullptr;
    }

    [[nodiscard]] JMapInfoIter child_obj_iter(const JMapInfoIter& rIter, int child_index) {
        const auto* info = child_info(rIter);
        if (info == nullptr || child_index < 0) {
            return {};
        }

        const auto parent_id = link_id(rIter);
        if (parent_id < 0) {
            return {};
        }

        auto matched_index = 0;
        for (auto entry_index = 0; entry_index < info->getNumEntries(); ++entry_index) {
            auto child_parent_id = s32{-1};
            if (!info->getValue(entry_index, "ParentID", &child_parent_id) || child_parent_id != parent_id) {
                continue;
            }

            if (matched_index == child_index) {
                return JMapInfoIter(info, entry_index);
            }
            ++matched_index;
        }

        return {};
    }
}  // namespace

namespace MR {
    bool isValidInfo(const JMapInfoIter& rIter) {
        return rIter.isValid();
    }

    bool isObjectName(const JMapInfoIter& rIter, const char* pName) {
        const char* object_name = nullptr;
        return getObjectName(&object_name, rIter) && is_equal_string_case(object_name, pName);
    }

    bool isEqualObjectName(const JMapInfoIter& rIter, const char* pName) {
        return isObjectName(rIter, pName);
    }

    bool getObjectName(const char** pDest, const JMapInfoIter& rIter) {
        if (pDest == nullptr || !rIter.isValid()) {
            return false;
        }

        if (rIter.getValue("type", pDest)) {
            return true;
        }

        return rIter.getValue("name", pDest);
    }

    bool isExistJMapArg(const JMapInfoIter& rIter) {
        auto value = s32{};
        return rIter.isValid() && rIter.getValue("Obj_arg0", &value);
    }

    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pFieldName, s32* pOut) {
        return get_arg_no_init(rIter, pFieldName, pOut);
    }

    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pFieldName, f32* pOut) {
        return get_arg_no_init(rIter, pFieldName, pOut);
    }

    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pFieldName, bool* pOut) {
        return get_arg_no_init(rIter, pFieldName, pOut);
    }

    bool getJMapInfoArg0WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 0U, pOut);
    }
    bool getJMapInfoArg0WithInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_with_init(rIter, 0U, pOut);
    }
    bool getJMapInfoArg0WithInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_with_init(rIter, 0U, pOut);
    }
    bool getJMapInfoArg1WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 1U, pOut);
    }
    bool getJMapInfoArg1WithInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_with_init(rIter, 1U, pOut);
    }
    bool getJMapInfoArg1WithInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_with_init(rIter, 1U, pOut);
    }
    bool getJMapInfoArg2WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 2U, pOut);
    }
    bool getJMapInfoArg2WithInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_with_init(rIter, 2U, pOut);
    }
    bool getJMapInfoArg2WithInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_with_init(rIter, 2U, pOut);
    }
    bool getJMapInfoArg3WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 3U, pOut);
    }
    bool getJMapInfoArg3WithInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_with_init(rIter, 3U, pOut);
    }
    bool getJMapInfoArg3WithInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_with_init(rIter, 3U, pOut);
    }
    bool getJMapInfoArg4WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 4U, pOut);
    }
    bool getJMapInfoArg4WithInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_with_init(rIter, 4U, pOut);
    }
    bool getJMapInfoArg5WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 5U, pOut);
    }
    bool getJMapInfoArg6WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 6U, pOut);
    }
    bool getJMapInfoArg7WithInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_with_init(rIter, 7U, pOut);
    }
    bool getJMapInfoArg7WithInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_with_init(rIter, 7U, pOut);
    }

    bool getJMapInfoArg0NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 0U, pOut);
    }
    bool getJMapInfoArg0NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 0U, pOut);
    }
    bool getJMapInfoArg0NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 0U, pOut);
    }
    bool getJMapInfoArg1NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 1U, pOut);
    }
    bool getJMapInfoArg1NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 1U, pOut);
    }
    bool getJMapInfoArg1NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 1U, pOut);
    }
    bool getJMapInfoArg2NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 2U, pOut);
    }
    bool getJMapInfoArg2NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 2U, pOut);
    }
    bool getJMapInfoArg2NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 2U, pOut);
    }
    bool getJMapInfoArg3NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 3U, pOut);
    }
    bool getJMapInfoArg3NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 3U, pOut);
    }
    bool getJMapInfoArg3NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 3U, pOut);
    }
    bool getJMapInfoArg4NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 4U, pOut);
    }
    bool getJMapInfoArg4NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 4U, pOut);
    }
    bool getJMapInfoArg4NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 4U, pOut);
    }
    bool getJMapInfoArg5NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 5U, pOut);
    }
    bool getJMapInfoArg5NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 5U, pOut);
    }
    bool getJMapInfoArg5NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 5U, pOut);
    }
    bool getJMapInfoArg6NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 6U, pOut);
    }
    bool getJMapInfoArg6NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 6U, pOut);
    }
    bool getJMapInfoArg6NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 6U, pOut);
    }
    bool getJMapInfoArg7NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return get_obj_arg_no_init(rIter, 7U, pOut);
    }
    bool getJMapInfoArg7NoInit(const JMapInfoIter& rIter, f32* pOut) {
        return get_obj_arg_no_init(rIter, 7U, pOut);
    }
    bool getJMapInfoArg7NoInit(const JMapInfoIter& rIter, bool* pOut) {
        return get_obj_arg_no_init(rIter, 7U, pOut);
    }

    bool getJMapInfoTrans(const JMapInfoIter& rIter, TVec3f* pOut) {
        return getJMapInfoTransLocal(rIter, pOut);
    }
    bool getJMapInfoRotate(const JMapInfoIter& rIter, TVec3f* pOut) {
        return getJMapInfoRotateLocal(rIter, pOut);
    }
    bool getJMapInfoTransLocal(const JMapInfoIter& rIter, TVec3f* pOut) {
        return get_vec3_components(rIter, "pos_x", "pos_y", "pos_z", pOut);
    }
    bool getJMapInfoRotateLocal(const JMapInfoIter& rIter, TVec3f* pOut) {
        return get_vec3_components(rIter, "dir_x", "dir_y", "dir_z", pOut);
    }
    bool getJMapInfoScale(const JMapInfoIter& rIter, TVec3f* pOut) {
        return get_vec3_components(rIter, "scale_x", "scale_y", "scale_z", pOut);
    }

    bool getJMapInfoV3f(const JMapInfoIter& rIter, const char* pName, TVec3f* pOut) {
        if (pName == nullptr || pOut == nullptr) {
            return false;
        }

        auto x = std::array< char, 64U >{};
        auto y = std::array< char, 64U >{};
        auto z = std::array< char, 64U >{};
        std::snprintf(x.data(), x.size(), "%sX", pName);
        std::snprintf(y.data(), y.size(), "%sY", pName);
        std::snprintf(z.data(), z.size(), "%sZ", pName);
        return get_vec3_components(rIter, x.data(), y.data(), z.data(), pOut);
    }

    bool getJMapInfoFollowID(const JMapInfoIter& rIter, s32* pOut) {
        return get_arg_and_init(rIter, "FollowId", pOut);
    }
    bool getJMapInfoGroupID(const JMapInfoIter& rIter, s32* pOut) {
        return get_arg_and_init(rIter, "GroupId", pOut) || getJMapInfoClippingGroupID(rIter, pOut);
    }
    bool getJMapInfoClippingGroupID(const JMapInfoIter& rIter, s32* pOut) {
        return get_arg_and_init(rIter, "ClippingGroupId", pOut);
    }
    bool getJMapInfoDemoGroupID(const JMapInfoIter& rIter, s32* pOut) {
        return get_arg_and_init(rIter, "DemoGroupId", pOut);
    }
    bool getJMapInfoLinkID(const JMapInfoIter& rIter, s32* pOut) {
        return pOut != nullptr && rIter.getValue("l_id", pOut);
    }
    bool getJMapInfoCameraSetID(const JMapInfoIter& rIter, s32* pOut) {
        return get_arg_and_init(rIter, "CameraSetId", pOut);
    }
    bool getJMapInfoViewGroupID(const JMapInfoIter& rIter, s32* pOut) {
        return get_arg_and_init(rIter, "ViewGroupId", pOut);
    }
    bool getJMapInfoMessageID(const JMapInfoIter& rIter, s32* pOut) {
        return get_arg_and_init(rIter, "MessageId", pOut);
    }

    bool isConnectedWithRail(const JMapInfoIter& rIter) {
        auto id = s32{};
        return get_arg_and_init(rIter, "CommonPath_ID", &id) && id != -1;
    }

    bool isExistStageSwitchA(const JMapInfoIter& rIter) {
        auto id = s32{};
        return get_arg_and_init(rIter, "SW_A", &id) && id != -1;
    }

    bool isExistStageSwitchB(const JMapInfoIter& rIter) {
        auto id = s32{};
        return get_arg_and_init(rIter, "SW_B", &id) && id != -1;
    }

    bool isExistStageSwitchAppear(const JMapInfoIter& rIter) {
        auto id = s32{};
        return get_arg_and_init(rIter, "SW_APPEAR", &id) && id != -1;
    }

    bool isExistStageSwitchDead(const JMapInfoIter& rIter) {
        auto id = s32{};
        return get_arg_and_init(rIter, "SW_DEAD", &id) && id != -1;
    }

    bool isExistStageSwitchSleep(const JMapInfoIter& rIter) {
        auto id = s32{};
        return get_arg_and_init(rIter, "SW_SLEEP", &id) && id != -1;
    }

    s32 getDemoGroupID(const JMapInfoIter& rIter) {
        auto id = s32{-1};
        (void)rIter.getValue("DemoGroupId", &id);
        return id;
    }

    s32 getDemoGroupLinkID(const JMapInfoIter& rIter) {
        auto id = s32{-1};
        (void)rIter.getValue("l_id", &id);
        return id;
    }

    s32 getDemoCastID(const JMapInfoIter& rIter) {
        auto id = s32{-1};
        (void)rIter.getValue("CastId", &id);
        return id;
    }

    const char* getDemoName(const JMapInfoIter& rIter) {
        const char* name = nullptr;
        (void)rIter.getValue("DemoName", &name);
        return name;
    }

    const char* getDemoSheetName(const JMapInfoIter& rIter) {
        const char* name = nullptr;
        (void)rIter.getValue("TimeSheetName", &name);
        return name;
    }

    s32 getChildObjNum(const JMapInfoIter& rIter) {
        const auto* info = child_info(rIter);
        if (info == nullptr) {
            return 0;
        }

        const auto parent_id = link_id(rIter);
        if (parent_id < 0) {
            return 0;
        }

        auto count = s32{};
        for (auto entry_index = 0; entry_index < info->getNumEntries(); ++entry_index) {
            auto child_parent_id = s32{-1};
            if (info->getValue(entry_index, "ParentID", &child_parent_id) && child_parent_id == parent_id) {
                ++count;
            }
        }
        return count;
    }

    const char* getChildObjName(const char** pDest, const JMapInfoIter& rIter, int index) {
        if (pDest == nullptr) {
            return nullptr;
        }

        *pDest = nullptr;
        const auto iter = child_obj_iter(rIter, index);
        (void)getObjectName(pDest, iter);
        return *pDest;
    }

    void initChildObj(NameObj* pObj, const JMapInfoIter& rIter, int index) {
        if (pObj == nullptr) {
            return;
        }

        const auto iter = child_obj_iter(rIter, index);
        pObj->init(iter);
    }
}  // namespace MR
