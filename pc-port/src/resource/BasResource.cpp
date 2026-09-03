#include "BasResource.hpp"
#include "JSystem/JAudio2/JAUSoundAnimator.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <bit>
#include <cmath>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>

JAUSoundAnimationControl::~JAUSoundAnimationControl() = default;

namespace {
struct Registry {
    std::mutex mutex;
    std::map<const void*, const JAUSoundAnimation*> aliases;
};
Registry& registry() { static Registry value; return value; }
std::uint16_t u16be(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return (std::uint16_t(bytes[offset]) << 8) | bytes[offset + 1];
}
std::uint32_t u32be(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return (std::uint32_t(u16be(bytes, offset)) << 16) | u16be(bytes, offset + 2);
}
float f32be(std::span<const std::uint8_t> bytes, std::size_t offset) {
    const auto value = std::bit_cast<float>(u32be(bytes, offset));
    if (!std::isfinite(value)) throw std::invalid_argument("BAS contains a nonfinite event value");
    return value;
}
}

namespace smgpc::resource {
struct BasResource::Storage final : JAUSoundAnimationControl {
    std::shared_ptr<const void> archive_owner;
    const void* raw_identity;
    std::vector<JAUSoundAnimationSound> sounds;
    JAUSoundAnimation value{};

    Storage(std::span<const std::uint8_t> bytes, std::shared_ptr<const void> owner)
        : archive_owner(std::move(owner)), raw_identity(bytes.data()) {
        if (!archive_owner || bytes.size() < 8)
            throw std::invalid_argument("BAS needs a retained archive and complete header");
        const auto count = u16be(bytes, 0);
        if (u32be(bytes, 4) != 0 || bytes.size() < 8 + std::size_t(count) * 32)
            throw std::invalid_argument("BAS has a serialized control pointer or truncated events");
        sounds.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto offset = 8 + 32 * i;
            auto& sound = sounds[i];
            sound.mSoundID = u32be(bytes, offset);
            sound.mNoteOnTime = f32be(bytes, offset + 4);
            sound.mNoteOffTime = f32be(bytes, offset + 8);
            sound.mBasePitch = f32be(bytes, offset + 12);
            sound.mFlags = u32be(bytes, offset + 16);
            sound.mBaseVolume = bytes[offset + 20];
            sound.mPitchDelta = std::bit_cast<std::int8_t>(bytes[offset + 21]);
            sound.mPlayTime = bytes[offset + 22];
            sound.mBasePan = bytes[offset + 23];
            sound.mVolumeDelta = std::bit_cast<std::int8_t>(bytes[offset + 24]);
            sound.mRepeatInterval = bytes[offset + 25];
            sound._1A = std::bit_cast<std::int8_t>(bytes[offset + 26]);
            sound._1C = u32be(bytes, offset + 28);
            if (sound.playsAtIntervals() && sound.mRepeatInterval == 0)
                throw std::invalid_argument("BAS interval event has zero repeat interval");
        }
        value.mNumSounds = count;
        value.mControl = this;
        auto& entries = registry();
        std::lock_guard lock(entries.mutex);
        if (entries.aliases.contains(raw_identity))
            throw std::logic_error("BAS source already has a resource owner");
        entries.aliases.emplace(raw_identity, &value);
        try { entries.aliases.emplace(&value, &value); }
        catch (...) { entries.aliases.erase(raw_identity); throw; }
    }
    ~Storage() override {
        auto& entries = registry();
        std::lock_guard lock(entries.mutex);
        entries.aliases.erase(raw_identity);
        entries.aliases.erase(&value);
    }
    JAUSoundAnimationSound* getSound(const JAUSoundAnimation*, int index) override { return &sounds[index]; }
    u16 getNumSounds(const JAUSoundAnimation*) override { return static_cast<u16>(sounds.size()); }
};

BasResource::BasResource(std::span<const std::uint8_t> source, std::shared_ptr<const void> owner) {
    compat::JkrHostAllocationScope host;
    _storage = std::make_unique<Storage>(source, std::move(owner));
}
BasResource::~BasResource() {
    compat::JkrHostAllocationScope host;
    _storage.reset();
}
BasResource::BasResource(BasResource&&) noexcept = default;
BasResource& BasResource::operator=(BasResource&& other) noexcept {
    compat::JkrHostAllocationScope host;
    _storage = std::move(other._storage);
    return *this;
}
const JAUSoundAnimation* BasResource::animation() const noexcept { return &_storage->value; }
const JAUSoundAnimation* resolve_bas_animation(const JAUSoundAnimation* identity) {
    if (identity == nullptr) return nullptr;
    auto& entries = registry();
    std::lock_guard lock(entries.mutex);
    const auto found = entries.aliases.find(identity);
    if (found == entries.aliases.end())
        throw std::invalid_argument("BAS resource was not registered by its retained archive owner");
    return found->second;
}
}
