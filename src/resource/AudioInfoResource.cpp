#include "resource/AudioInfoResource.hpp"
#include "resource/RemixSequenceResource.hpp"
#include "JSystem/JKernel/JKRArchive.hpp"
#include "Game/RhythmLib/AudChordInfo.hpp"
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>

namespace smgpc::resource {
namespace {
using Bytes = std::span<const std::uint8_t>;
void require(Bytes bytes, std::size_t offset, std::size_t size) {
    if (offset > bytes.size() || size > bytes.size() - offset)
        aurora::throw_host_exception<std::runtime_error>("Audio info record is outside its resource");
}
template<class T> T read(Bytes bytes, std::size_t offset) { return aurora::endian::read_big<T>(bytes, offset); }
template<class T> void write(std::vector<std::uint8_t>& bytes, std::size_t offset, T value) {
    require(bytes, offset, sizeof(T));
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}
Bytes resource(JKRArchive* archive, std::uint16_t id) {
    auto* data = static_cast<const std::uint8_t*>(archive->getResource(id));
    const auto size = archive->getResSize(data);
    if (!data || size == std::numeric_limits<std::uint32_t>::max())
        aurora::throw_host_exception<std::runtime_error>("Audio info archive resource is missing");
    return {data, size};
}
std::vector<std::uint8_t> chord(Bytes source) {
    require(source, 0, 16);
    if (std::memcmp(source.data() + 4, "CITS", 4))
        aurora::throw_host_exception<std::runtime_error>("Invalid chord table signature");
    const auto chords = read<std::uint16_t>(source, 12);
    const auto scales = read<std::uint16_t>(source, 14);
    require(source, 16, (chords + scales) * 4U);
    const std::size_t scaleBase = 16 + (chords + scales) * sizeof(void*);
    const std::size_t payload = scaleBase + scales * sizeof(AudScaleData);
    std::vector<std::uint8_t> out(payload + source.size());
    std::memcpy(out.data() + payload, source.data(), source.size());
    std::memcpy(out.data() + 4, "CITS", 4);
    write(out, 0, std::uint32_t{1}); // pointers below are already relocated
    write(out, 12, chords); write(out, 14, scales);
    for (unsigned i = 0; i < chords; ++i) {
        const auto offset = read<std::uint32_t>(source, 16 + i * 4);
        require(source, offset, sizeof(AudChordData));
        write(out, 16 + i * sizeof(void*), out.data() + payload + offset);
    }
    for (unsigned i = 0; i < scales; ++i) {
        const auto offset = read<std::uint32_t>(source, 16 + (chords + i) * 4);
        const auto up = read<std::uint32_t>(source, offset);
        const auto down = read<std::uint32_t>(source, offset + 4);
        require(source, up, 12); require(source, down, 12);
        write(out, 16 + (chords + i) * sizeof(void*), out.data() + scaleBase + i * sizeof(AudScaleData));
        const AudScaleData scale{out.data() + payload + up, out.data() + payload + down};
        write(out, scaleBase + i * sizeof(scale), scale);
    }
    return out;
}
std::vector<std::uint8_t> melody_sequences(Bytes source) {
    const auto count = read<std::uint32_t>(source, 0);
    require(source, 4, std::size_t(count) * 4);
    std::vector<std::uint8_t> out(source.begin(), source.end());
    write(out, 0, count);
    for (unsigned i = 0; i < count; ++i) {
        const auto offset = read<std::uint32_t>(source, 4 + i * 4);
        if (offset != 0xffffffff) require(source, offset, 1);
        write(out, 4 + i * 4, offset);
    }
    return out;
}
std::vector<std::uint8_t> melody_params(Bytes source) {
    const auto count = read<std::uint32_t>(source, 0);
    const auto params = read<std::uint32_t>(source, 4);
    const auto names = read<std::uint32_t>(source, 8);
    require(source, params, std::size_t(count) * 8);
    require(source, names, std::size_t(count) * 4);
    const auto nativeNames = (source.size() + alignof(void*) - 1) & ~(alignof(void*) - 1);
    std::vector<std::uint8_t> out(nativeNames + count * sizeof(void*));
    std::memcpy(out.data(), source.data(), source.size());
    write(out, 0, count); write(out, 4, params); write(out, 8, std::uint32_t(nativeNames));
    for (unsigned i = 0; i < count; ++i) {
        write(out, params + i * 8 + 4, read<std::uint16_t>(source, params + i * 8 + 4));
        const auto offset = read<std::uint32_t>(source, names + i * 4);
        require(source, offset, 1);
        if (!std::memchr(source.data() + offset, 0, source.size() - offset))
            aurora::throw_host_exception<std::runtime_error>("Unterminated melody name");
        write(out, nativeNames + i * sizeof(void*), std::uintptr_t(offset));
    }
    return out;
}
}
void AudioInfoResources::publish(JKRArchive* archive, std::uint16_t id, std::vector<std::uint8_t> bytes) {
    mData.push_back(std::move(bytes));
    const auto* entry = archive->findIdResource(id);
    mOverrides.push_back(archive->overrideNativeResource(static_cast<u32>(entry - archive->mFiles), mData.back().data()));
}
AudioInfoResources::AudioInfoResources(JKRArchive* chords, JKRArchive* melodies, JKRArchive* remix) {
    if (chords) {
        for (u32 i = 0; i < chords->mInfoBlock->mNrFiles; ++i) {
            const auto* entry = chords->findIdxResource(i);
            if (entry && (entry->mFlag & 1)) publish(chords, entry->mFileID, chord(resource(chords, entry->mFileID)));
        }
    }
    if (melodies) {
        publish(melodies, 0, melody_sequences(resource(melodies, 0)));
        publish(melodies, 1, melody_params(resource(melodies, 1)));
    }
    if (remix) {
        const auto words = decode_remix_sequence(resource(remix, 0));
        std::vector<std::uint8_t> bytes(words.size() * sizeof(u32));
        std::memcpy(bytes.data(), words.data(), bytes.size());
        publish(remix, 0, std::move(bytes));
    }
}
}
