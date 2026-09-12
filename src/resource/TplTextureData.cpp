#include "resource/TplTextureData.hpp"
#include "resource/TplTexture.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/exception.hpp>
#include <cstring>
#include <map>
#include <stdexcept>

namespace smgpc::resource {
    TplTextureData::TplTextureData(std::span<const std::uint8_t> bytes, std::shared_ptr<Mem1ResourceHeap> heap) {
        compat::JkrHostAllocationScope host;
        const auto source = read_tpl_palette(bytes);
        if (source.descriptors.empty())
            aurora::throw_host_exception<std::invalid_argument>("Native TPL requires at least one texture descriptor");
        char* base;
        if (heap) {
            _allocation = heap->allocate(bytes.size());
            std::memcpy(_allocation.bytes().data(), bytes.data(), bytes.size());
            base = reinterpret_cast<char*>(_allocation.bytes().data());
        } else {
            _host_bytes.assign(bytes.begin(), bytes.end());
            base = reinterpret_cast<char*>(_host_bytes.data());
        }
        _headers.resize(source.descriptors.size());
        _cluts.resize(source.descriptors.size());
        _descriptors.resize(source.descriptors.size());
        std::map<u32, TPLHeader*> header_aliases;
        std::map<u32, TPLClutHeader*> clut_aliases;
        for (std::size_t i = 0; i < source.descriptors.size(); ++i) {
            const auto& d = source.descriptors[i];
            _headers[i] = {d.height, d.width, static_cast<u32>(d.format), base + d.image_data_offset,
                GXTexWrapMode(d.wrap_s), GXTexWrapMode(d.wrap_t), GXTexFilter(d.min_filter), GXTexFilter(d.mag_filter),
                d.lod_bias, d.edge_lod_enable, d.min_lod, d.max_lod, 1};
            _descriptors[i].textureHeader = header_aliases.try_emplace(d.texture_header_offset, &_headers[i]).first->second;
            if (d.has_palette) {
                _cluts[i] = {d.palette_entry_count, 1, 0, GXTlutFmt(d.palette_format), base + d.palette_data_offset};
                _descriptors[i].CLUTHeader = clut_aliases.try_emplace(d.clut_header_offset, &_cluts[i]).first->second;
            }
        }
        _palette = {source.version, source.descriptor_count, _descriptors.data()};
    }
}
