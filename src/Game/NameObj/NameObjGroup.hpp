#pragma once

#include "Game/NameObj/NameObj.hpp"

/// @brief Class that can contain multiple NameObj instances stored in a group.
class NameObjGroup : public NameObj {
public:
    /// @brief Creates a new `NameObjGroup`.
    /// @param pName A pointer to the null-terminated name of the object.
    /// @param numMax The maximum allowed number of `NameObj` instances in the group.
    NameObjGroup(const char* pName, int numMax);

    /// @brief Destroys the `NameObjGroup`.
    virtual ~NameObjGroup();

    NameObj* getObj(int index) const {
        return mObjects[index];
    }

    s32 getObjectCount() const {
        return mObjectCount;
    }

    void registerObj(NameObj*);
    void pauseOffAll() const;
    void initObjArray(int);

    /* 0x0C */ s32 mObjectNumMax;
    /* 0x10 */ s32 mObjectCount;
    /* 0x14 */ NameObj** mObjects;
};
