#pragma once

#include "Game/Util/JMapInfo.hpp"
#include <revolution/types.h>
#include <vector>

class NameObjHolder;
class HitSensor;

/// @brief The most basic form of an object.
class NameObj {
public:
    /// @brief Creates a new `NameObj`.
    /// @param pName A pointer to the null-terminated name of the object.
    NameObj(const char* pName);

    /// @brief Destroys the `NameObj`.
    /* 0x08 */ virtual ~NameObj();

    /// @brief Intializes the `NameObj` while being placed into a scene.
    /// @param rIter A reference to an iterator over a `JMapInfo`.
    /* 0x0C */ virtual void init(const JMapInfoIter& rIter);

    /// @brief Intializes the `NameObj` after being placed into a scene.
    /* 0x10 */ virtual void initAfterPlacement();

    /* 0x14 */ virtual void movement();

    /// @brief Draws the `NameObj` to the screen.
    /* 0x18 */ virtual void draw() const;

    /* 0x1C */ virtual void calcAnim();
    /* 0x20 */ virtual void calcViewAndEntry();

    /// @brief Initializes the `NameObj` without an iterator over a `JMapInfo`.
    void initWithoutIter();

    /// @brief Updates the name of the `NameObj`.
    /// @param pName A pointer to the new null-terminated name of the object.
    void setName(const char* pName);

    /// @brief Returns the name of the `NameObj`.
    /// @return A pointer to the null-terminated name of the object.
    const char* getName() const {
        return mName;
    }

    u16 getFlag() const {
        return mFlag;
    }

    void executeMovement();
    void requestSuspend();
    void requestResume();
    void syncWithFlags();
    void detachNativeHolder() noexcept;

    struct NativeRegistrationMarker {
        u64 mNextGeneration;
    };
    using NativeRegistrationFilter = bool (*)(const NameObj*, const void*) noexcept;

    static u64 nativeGeneration(const NameObj*) noexcept;
    static bool isNativeOwnershipClaimed(const NameObj*) noexcept;
    void claimNativeOwnership(const void*);
    void retireNativeLifetime() noexcept;
    static NativeRegistrationMarker markNativeRegistrations() noexcept;
    static std::vector< NameObj* > snapshotNativeObjects();
    static std::vector< NameObj* > snapshotNativeObjectsSince(NativeRegistrationMarker);
    static NameObj* newestNativeObjectSince(NativeRegistrationMarker, NativeRegistrationFilter = nullptr,
                                           const void* context = nullptr) noexcept;
    static bool wasNativeRegisteredSince(const NameObj*, NativeRegistrationMarker) noexcept;
    virtual void releaseNativeReference(const NameObj*) noexcept;
    static void notifyNativeSensorRetirement(const HitSensor*) noexcept;
    virtual void releaseNativeSensorReference(const HitSensor*) noexcept;

    /* 0x04 */ const char* mName;  ///< A string to identify the NameObj.
    /* 0x08 */ u16 mFlag;          ///< Flags in relation to movement.
    /* 0x0A */ s16 mExecutorIdx;   ///< The index into the NameObjExecuteInfo array.

private:
    friend class NameObjHolder;
    struct NativeIteration;
    static NameObj* sNativeFirst;
    static NameObj* sNativeLast;
    static u64 sNativeNextGeneration;
    static NativeIteration* sNativeIteration;

    NameObjHolder* mNativeHolder = nullptr;
    NameObj* mNativePrevious = nullptr;
    NameObj* mNativeNext = nullptr;
    u64 mNativeGeneration = 0;
    const void* mNativeOwner = nullptr;
};

/// @brief Contains static functions to begin and end movement in a NameObj.
class NameObjFunction {
public:
    static void requestMovementOn(NameObj*);
    static void requestMovementOff(NameObj*);
};
