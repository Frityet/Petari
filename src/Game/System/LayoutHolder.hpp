#pragma once

#include "Game/System/ResourceInfo.hpp"
#include <nw4r/lyt/resourceAccessor.h>
#include <memory>
#include <filesystem>

class JKRArchive;
class JKRFileFinder;
class JKRHeap;
namespace smgpc::resource {
    class RarcArchive;
}

class LayoutHolder : public nw4r::lyt::ResourceAccessor {
public:
    LayoutHolder(JKRArchive&);
    LayoutHolder(const LayoutHolder&) = delete;
    LayoutHolder& operator=(const LayoutHolder&) = delete;

    [[nodiscard]] std::shared_ptr<const void> retainNativeResources() const;
    [[nodiscard]] std::size_t nativeArchiveReferenceCount() const noexcept;
    void ensureNativeResourcesUnborrowed() const;
    [[nodiscard]] const smgpc::resource::RarcArchive& nativeResourceSource() const;
    [[nodiscard]] const std::filesystem::path& nativeResourcePath() const;
    [[nodiscard]] JKRHeap& heap() const noexcept;

    virtual ~LayoutHolder();
    virtual void* GetResource(u32, const char*, u32*);
    virtual nw4r::ut::Font* GetFont(const char*);
    virtual void* getResOther(const char*) const;
    virtual u32 getResOtherNum() const;
    virtual const char* getResOtherName(u32) const;
    virtual void* getResOther(u32) const;
    virtual bool isExistResOther(const char*) const;

    bool isAnimationHashEqual(u32, u32) const;
    void initializeArc();
    JKRFileFinder* getFileFinder(const char*);
    u32 initEachResTable(ResTable*, const char* const*);

    s32 count(const char*, const char*);
    void mount(char*);
    ResFileInfo* createAndRegisterObject(const char*, void*);

    JKRArchive* mArchive;  // 0x4
    ResTable mLayoutRes;   // 0x8
    ResTable mAnimRes;     // 0x10
    ResTable mResOther;    // 0x18

private:
    struct NativeResources;
    std::shared_ptr<NativeResources> mNativeResources;
    void destroyNativeResources() noexcept;
};
