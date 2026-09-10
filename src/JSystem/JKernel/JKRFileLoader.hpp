#pragma once

#include "JSystem/JKernel/JKRDisposer.hpp"

class JKRArcFinder;

class JKRFileLoader : public JKRDisposer {
public:
    JKRFileLoader();
    ~JKRFileLoader() override;
    virtual void unmount();

    // The native SDK exposes read-only resource lookup through const loaders.
    virtual void* getResource(const char*) const = 0;
    virtual void* getResource(u32, const char*) const = 0;
    virtual u32 readResource(void*, u32, const char*) const = 0;
    virtual u32 getResSize(const void*) const = 0;
    virtual s32 countFile(const char*) const = 0;
    virtual JKRArcFinder* getFirstFile(const char*) const = 0;

    static void* getGlbResource(const char*, JKRFileLoader* = nullptr);
    static void initializeVolumeList();
    void prependVolumeList(JSULink<JKRFileLoader>*);
    void removeVolumeList(JSULink<JKRFileLoader>*);

    static JSUList<JKRFileLoader> sFileLoaderList;
    static JSUList<JKRFileLoader>& sVolumeList;
    static JKRFileLoader* gCurrentFileLoader;

    JSULink<JKRFileLoader> mLoaderLink;
    char* mLoaderName;
    u32 mLoaderType;
    bool mIsMounted;
    u32 _34;
};
