#include "layout/LytTexMap.hpp"
#include <aurora/allocation.hpp>
#include "resource/BtiTextureData.hpp"
#include "resource/TplTextureData.hpp"
#include <aurora/exception.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace smgpc::layout {
    namespace {
        struct HostBtiStorage final {
            explicit HostBtiStorage(std::span<const std::uint8_t> source)
                : header(resource::decode_texture_image_header(source)) {
                resource::validate_texture_image(source, 0, header);
                bytes.assign(source.begin(), source.end());
            }
            ResTIMG header;
            std::vector<std::uint8_t> bytes;
        };
    }

    nw4r::lyt::TexMap make_tex_map(std::string_view name, std::span<const std::uint8_t> bytes,
                                  std::shared_ptr<resource::Mem1ResourceHeap> heap) {
        aurora::allocation::HostAllocationScope host;
        auto lower = std::string(name);
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        nw4r::lyt::TexMap result;
        std::shared_ptr<const void> backing;
        const TPLPalette* tpl_palette = nullptr;
        if (lower.ends_with(".tpl")) {
            auto texture = std::make_shared<resource::TplTextureData>(bytes, std::move(heap));
            const auto& palette = texture->palette();
            tpl_palette = &palette;
            const auto& descriptor = palette.descriptorArray[0];
            const auto& header = *descriptor.textureHeader;
            result.ReplaceImage(&descriptor);
            result.SetWrapMode(header.wrapS, header.wrapT);
            result.SetFilter(header.minFilter, header.magFilter);
            result.SetLOD(header.minLOD, header.maxLOD);
            result.SetLODBias(header.LODBias);
            result.SetEdgeLODEnable(header.edgeLODEnable);
            result.SetMipMap(header.maxLOD != 0);
            backing = std::move(texture);
        } else if (lower.ends_with(".bti")) {
            const ResTIMG* header;
            const std::uint8_t* base;
            if (heap) {
                auto texture = std::make_shared<resource::BtiTextureData>(bytes, std::move(heap));
                header = texture->image();
                base = reinterpret_cast<const std::uint8_t*>(header);
                backing = std::move(texture);
            } else {
                auto texture = std::make_shared<HostBtiStorage>(bytes);
                header = &texture->header;
                base = texture->bytes.data();
                backing = std::move(texture);
            }
            result.SetImage(const_cast<std::uint8_t*>(base + header->mImageDataOffset));
            result.SetSize(header->mWidth, header->mHeight);
            result.SetTexelFormat(GXTexFmt(header->mFormat));
            result.SetWrapMode(GXTexWrapMode(header->mWrapS), GXTexWrapMode(header->mWrapT));
            result.SetFilter(GXTexFilter(header->mMinType), GXTexFilter(header->mMagType));
            result.SetMipMap(header->mMipmap);
            result.SetLOD(header->mMinLod / 8.0F, header->mMaxLod / 8.0F);
            result.SetLODBias(header->mLodBias / 100.0F);
            result.SetBiasClampEnable(header->mBiasClamp);
            result.SetEdgeLODEnable(header->mDoEdgeLod);
            result.SetAnisotropy(GXAnisotropy(header->mMaxAnisotropy));
            if (header->mPaletteNum) {
                result.SetPalette(const_cast<std::uint8_t*>(base + header->mPaletteDataOffset));
                result.SetPaletteFormat(GXTlutFmt(header->mPaletteFormat));
                result.SetPaletteEntryNum(header->mPaletteNum);
            }
        } else {
            aurora::throw_host_exception<std::invalid_argument>("Layout texture requires a TPL or BTI resource: " + std::string(name));
        }
        result.SetHostResourceState(std::make_shared<nw4r::lyt::HostTextureResourceState>(
            nw4r::lyt::HostTextureResourceState{std::string(name), std::move(backing), tpl_palette}));
        return result;
    }

    std::string tex_map_name(const nw4r::lyt::TexMap& texture) {
        aurora::allocation::HostAllocationScope host;
        if (const auto& resource = texture.GetHostResourceState()) return resource->name;
        return "GX-image-" + std::to_string(reinterpret_cast<std::uintptr_t>(texture.mImage));
    }

    resource::DecodedTexture decode_tex_map(const nw4r::lyt::TexMap& texture) {
        aurora::allocation::HostAllocationScope host;
        if (!texture.mImage || !texture.mWidth || !texture.mHeight)
            aurora::throw_host_exception<std::invalid_argument>("Layout rendering requires an initialized SDK TexMap image");
        const auto format = static_cast<resource::TplTextureFormat>(texture.GetTexelFormat());
        const auto size = resource::gx_texture_data_size(texture.mWidth, texture.mHeight, format);
        const auto image = std::span(static_cast<const std::uint8_t*>(texture.mImage), size);
        const auto palette = std::span(static_cast<const std::uint8_t*>(texture.GetPalette()),
            texture.GetPalette() ? static_cast<std::size_t>(texture.GetPaletteEntryNum()) * 2U : 0U);
        return resource::decode_raw_gx_texture(image, texture.mWidth, texture.mHeight, format,
                                               palette, texture.GetPaletteFormat());
    }
}
