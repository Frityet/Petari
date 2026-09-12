#include "layout/NativeLayoutResource.hpp"
#include "layout/LytTexMap.hpp"
#include <aurora/allocation.hpp>
#include <aurora/endian.hpp>
#include "nw4r/lyt/arcResourceAccessor.h"
#include <cstdio>
#include <cstring>
#if defined(_WIN32)
#define stricmp _stricmp
#else
#include <strings.h>
#define stricmp strcasecmp
#endif
#include <revolution/arc.h>

namespace {
    s32 FindNameResource(ARCHandle* pHandle, const char* pResName) {
        s32 entryNum = -1;
        ARCDir dir;
        ARCOpenDir(pHandle, ".", &dir);
        ARCDirEntry dirEntry;
        while (ARCReadDir(&dir, &dirEntry)) {
            if (dirEntry.isDir) {
                ARCChangeDir(pHandle, dirEntry.name);
                entryNum = FindNameResource(pHandle, pResName);
                ARCChangeDir(pHandle, "..");
                if (entryNum != -1) {
                    break;
                }
            } else {
                if (0 == stricmp(pResName, dirEntry.name)) {
                    entryNum = dirEntry.entryNum;
                    break;
                }
            }
        }

        ARCCloseDir(&dir);
        return entryNum;
    }

    void* GetResourceSub(ARCHandle* pHandle, const char* pRoot, nw4r::lyt::ResType resType, const char* pName, u32* pSize) {
        s32 entryNum = -1;
        if (-1 != ARCConvertPathToEntrynum(pHandle, pRoot)) {
            if (ARCChangeDir(pHandle, pRoot)) {
                if (resType == 0) {
                    entryNum = FindNameResource(pHandle, pName);
                } else {
                    char resTypeStr[5];
                    resTypeStr[0] = char(resType >> 24);
                    resTypeStr[1] = char(resType >> 16);
                    resTypeStr[2] = char(resType >> 8);
                    resTypeStr[3] = char(resType >> 0);
                    resTypeStr[4] = 0;

                    if (-1 != ARCConvertPathToEntrynum(pHandle, resTypeStr)) {
                        if (ARCChangeDir(pHandle, resTypeStr)) {
                            entryNum = ARCConvertPathToEntrynum(pHandle, pName);
                            ARCChangeDir(pHandle, "..");
                        }
                    }
                }

                ARCChangeDir(pHandle, "..");
            }
        }

        if (entryNum != -1) {
            ARCFileInfo fileInfo;
            ARCFastOpen(pHandle, entryNum, &fileInfo);
            void* res = ARCGetStartAddrInMem(&fileInfo);
            if (pSize) {
                *pSize = ARCGetLength(&fileInfo);
            }
            ARCClose(&fileInfo);
            return res;
        }

        return nullptr;
    }
};  // namespace

namespace nw4r {
    namespace lyt {
        namespace detail {
            nw4r::ut::Font* FindFont(FontRefList* pList, const char* pName) {
                for (FontRefList::Iterator it = pList->GetBeginIter(); it != pList->GetEndIter(); it++) {
                    if (0 == strcmp(pName, it->GetFontName())) {
                        return it->GetFont();
                    }
                }

                return nullptr;
            }
        };  // namespace detail

        ArcResourceAccessor::ArcResourceAccessor() : mArcBuf(nullptr) {
        }

        bool ArcResourceAccessor::Attach(void* pArchive, const char* pRoot) {
            BOOL succcess = ARCInitHandle(pArchive, &mArcHandle);
            if (!succcess) {
                return false;
            }

            mHostTextures.clear();
            mArcBuf = pArchive;
            strncpy(mResRootDir, pRoot, 127);
            mResRootDir[127] = '\0';
            return true;
        }

        void* ArcResourceAccessor::GetResource(ResType type, const char* pName, u32* pSize) {
            const aurora::allocation::HostAllocationScope host;
            const auto key = std::to_string(type) + ":" + pName;
            if (const auto found = mHostTextures.find(key); found != mHostTextures.end()) {
                if (pSize) *pSize = sizeof(TPLPalette);
                return const_cast<TPLPalette*>(found->second->tpl_palette);
            }
            u32 size = 0;
            void* resource = GetResourceSub(&mArcHandle, mResRootDir, type, pName, &size);
            if (resource == nullptr) return nullptr;
            const auto bytes = std::span(static_cast<const std::uint8_t*>(resource), size);
            if (size >= 4) {
                const auto signature = aurora::endian::read_big<u32>(bytes, 0);
                if (signature == 0x0020AF30) {
                    auto texture = smgpc::layout::make_tex_map(pName, bytes, nullptr);
                    auto state = texture.GetHostResourceState();
                    mHostTextures.emplace(key, state);
                    if (pSize) *pSize = sizeof(TPLPalette);
                    return const_cast<TPLPalette*>(state->tpl_palette);
                }
                if (signature == 'RLYT' || signature == 'RLAN' || signature == 'RFNT')
                    smgpc::layout::NativeLayoutResource::validate_archive_span(bytes);
            }
            if (pSize) *pSize = size;
            return resource;
        }

        std::shared_ptr<const HostTextureResourceState> ArcResourceAccessor::GetHostTextureResourceState(const char* name) {
            const aurora::allocation::HostAllocationScope host;
            GetResource('timg', name, nullptr);
            const auto found = mHostTextures.find(std::to_string(ResType('timg')) + ":" + name);
            return found == mHostTextures.end() ? nullptr : found->second;
        }

        ut::Font* ArcResourceAccessor::GetFont(const char* pName) {
            return detail::FindFont(&mFontList, pName);
        }

        ArcResourceAccessor::~ArcResourceAccessor() {
        }
    };  // namespace lyt
};  // namespace nw4r
