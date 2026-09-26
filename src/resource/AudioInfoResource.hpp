#pragma once
#include <cstdint>
#include <memory>
#include <vector>
class JKRArchive;
namespace smgpc::resource {
// Owns native pointer tables for the original rhythm/remix consumers.
class AudioInfoResources {
public:
    AudioInfoResources(JKRArchive* chords, JKRArchive* melodies, JKRArchive* remix);
private:
    void publish(JKRArchive*, std::uint16_t, std::vector<std::uint8_t>);
    std::vector<std::vector<std::uint8_t>> mData;
    std::vector<std::shared_ptr<const void>> mOverrides;
};
}
