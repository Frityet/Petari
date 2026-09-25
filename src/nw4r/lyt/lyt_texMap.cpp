#include "nw4r/lyt/common.h"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "nw4r/lyt/texMap.h"
#include "revolution/gx/GXEnum.h"
#include "revolution/gx/GXGet.h"
#include "revolution/gx/GXStruct.h"
#include "revolution/tpl.h"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace nw4r {
    namespace lyt {
        void TexMap::RegisterNativeLifetime() {
            JKRHeap::registerFinalizer(this, [](void* object) noexcept {
                static_cast<TexMap*>(object)->~TexMap();
            });
        }

        TexMap::~TexMap() {
            JKRHeap::unregisterFinalizer(this);
        }

        void TexMap::Get(_GXTexObj* pTexObj) const {
            if (detail::IsCITexelFormat(GetTexelFormat())) {
                u32 tlutName = GXGetTexObjTlut(pTexObj);
                GXInitTexObjCI(pTexObj, mImage, mWidth, mHeight, GXCITexFmt(GetTexelFormat()), GetWrapModeS(), GetWrapModeT(), IsMipMap(), tlutName);
            } else {
                GXInitTexObj(pTexObj, mImage, mWidth, mHeight, GetTexelFormat(), GetWrapModeS(), GetWrapModeT(), IsMipMap());
            }

            GXInitTexObjLOD(pTexObj, GetMinFilter(), GetMagFilter(), GetMinLOD(), GetMaxLOD(), GetLODBias(), IsBiasClampEnable(), IsEdgeLODEnable(),
                            GetAnisotropy());
        }

        void TexMap::Get(_GXTlutObj* pTlutObj) const {
            GXInitTlutObj(pTlutObj, GetPalette(), GetPaletteFormat(), GetPaletteEntryNum());
        }

        void TexMap::Set(const GXTexObj& texObj) {
            void* image;
            u16 width, height;
            GXTexFmt format;
            GXTexWrapMode wrapS, wrapT;
            GXBool mipmap;

            GXGetTexObjAll(&texObj, &image, &width, &height, &format, &wrapS, &wrapT, &mipmap);

            mImage = image;
            SetSize(width, height);
            mBits.textureFormat = format;
            SetWrapMode(wrapS, wrapT);
            SetMipMap(mipmap);

            GXTexFilter minFilter, magFilter;
            f32 minLOD, maxLOD, lodBias;
            GXBool biasCLampEnable, edgeLODEnable;
            GXAnisotropy aniso;
            GXGetTexObjLODAll(&texObj, &minFilter, &magFilter, &minLOD, &maxLOD, &lodBias, &biasCLampEnable, &edgeLODEnable, &aniso);

            SetFilter(minFilter, magFilter);
            SetLOD(minLOD, maxLOD);
            SetLODBias(lodBias);
            SetBiasClampEnable(biasCLampEnable);
            SetEdgeLODEnable(edgeLODEnable);
            mBits.anisotropy = aniso;
        }

        void TexMap::ReplaceImage(const TPLDescriptor* pTPLDesc) {
            const TPLHeader& header = *pTPLDesc->textureHeader;
            mImage = header.data;
            SetSize(header.width, header.height);
            SetTexelFormat(GXTexFmt(header.format));

            if (const TPLClutHeader* const pClut = pTPLDesc->CLUTHeader) {
                SetPalette(pClut->data);
                SetPaletteFormat(pClut->format);
                SetPaletteEntryNum(pClut->numEntries);
            } else {
                SetPalette(nullptr);
                SetPaletteFormat(GXTlutFmt(0));
                SetPaletteEntryNum(0);
            }
        }

        void TexMap::ReplaceImage(TPLPalette* p, u32 id) {
            // Packed TPL offsets must first be decoded by the bounded resource
            // owner. A native pointer's numeric value cannot identify Wii data.
            if (!p || p->versionNumber != 0x0020AF30 || !p->numDescriptors || !p->descriptorArray)
                aurora::throw_host_exception<std::invalid_argument>("TexMap requires a native TPL descriptor owner");
            ReplaceImage(&p->descriptorArray[id % p->numDescriptors]);
        }
    };  // namespace lyt
};  // namespace nw4r
