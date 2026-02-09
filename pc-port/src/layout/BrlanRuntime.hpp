#pragma once

#include "layout/BrlytRuntime.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pcport {

enum class BrlanCurveType : std::uint8_t {
    Step = 1,
    Hermite = 2,
};

struct BrlanKey {
    float frame = 0.0F;
    float value = 0.0F;
    float slope = 0.0F;
};

struct BrlanChannel {
    std::string paneName;
    std::string kind;
    std::uint16_t target = 0;
    BrlanCurveType curveType = BrlanCurveType::Hermite;
    std::vector<BrlanKey> keys;

    float Sample(float frame) const;
};

class BrlanAnimation {
public:
    static BrlanAnimation LoadFromFile(const std::filesystem::path& path);

    const std::string& GetName() const;
    int GetFrameSize() const;
    bool IsLooped() const;

    void ApplyToLayout(BrlytLayout& layout, float frame) const;

private:
    float NormalizeFrame(float frame) const;

    std::string mName;
    int mFrameSize = 0;
    bool mLoop = false;
    std::vector<BrlanChannel> mChannels;
};

class BrlanBundle {
public:
    static BrlanBundle LoadFromDirectory(const std::filesystem::path& animDir);

    const BrlanAnimation* FindByName(std::string_view name) const;
    const std::vector<BrlanAnimation>& GetAnimations() const;

private:
    std::vector<BrlanAnimation> mAnimations;
    std::unordered_map<std::string, int> mIndexByLowerName;
};

}  // namespace pcport
