#pragma once

#include "layout/BrlytRuntime.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct SDL_Window;

namespace pcport {

enum class CaptureSurface {
    Internal,
    Presented,
};

class BgfxRenderer {
public:
    static constexpr std::size_t kMaxCaptureLayers = 8U;

    BgfxRenderer(int width, int height);
    BgfxRenderer(SDL_Window* window, int width, int height);
    ~BgfxRenderer();

    BgfxRenderer(const BgfxRenderer&) = delete;
    BgfxRenderer& operator=(const BgfxRenderer&) = delete;

    int GetWidth() const;
    int GetHeight() const;

    void Resize(int width, int height);

    void BeginFrame();
    void Clear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255U);
    void RenderLayout(const BrlytLayout& layout);
    void EndFrame();

    void SetDebugOverlayEnabled(bool enabled);
    bool GetDebugOverlayEnabled() const;

    int GetCapturedLayerCount() const;

    std::uint64_t ComputeHash(CaptureSurface surface = CaptureSurface::Presented);
    std::uint64_t ComputeLayerHash(int layerIndex);
    bool SavePpm(const std::filesystem::path& path, CaptureSurface surface = CaptureSurface::Presented);
    bool SavePng(const std::filesystem::path& path, CaptureSurface surface = CaptureSurface::Presented);
    bool SaveLayerPpm(const std::filesystem::path& path, int layerIndex);
    bool SaveLayerPng(const std::filesystem::path& path, int layerIndex);
    bool SaveAllLayersPpm(const std::filesystem::path& directory, const std::string& baseName);
    bool SaveAllLayersPng(const std::filesystem::path& directory, const std::string& baseName);
    bool SaveDebugPaneList(const std::filesystem::path& path) const;

private:
    struct TransformContext {
        float originX = 0.0F;
        float originY = 0.0F;
        float scaleX = 1.0F;
        float scaleY = 1.0F;
        float alpha = 1.0F;
        bool visible = true;
    };

    void Initialize(int width, int height);
    void Shutdown();

    void EnsureOutputTargets(int width, int height);
    void RenderPaneRecursive(const BrlytLayout& layout, int paneIndex, const TransformContext& parent, std::uint8_t viewId,
                             std::uint64_t* hashAccumulator, bool collectDebugPaneRecords);
    void DrawPaneQuad(const PaneResource& pane, const BrlytLayout& layout, float drawX, float drawY, float drawW, float drawH, float alpha,
                      std::uint8_t viewId, std::uint64_t* hashAccumulator, bool collectDebugPaneRecords);
    void DrawDebugOutline(float x0, float y0, float x1, float y1, std::uint32_t abgrColor, std::uint8_t viewId);

    bool ReadSurface(CaptureSurface surface, std::vector<std::uint8_t>* pixels, int* width, int* height);
    bool ReadLayerSurface(int layerIndex, std::vector<std::uint8_t>* pixels, int* width, int* height);

    struct DebugPaneRecord {
        std::string name;
        float x = 0.0F;
        float y = 0.0F;
        float width = 0.0F;
        float height = 0.0F;
        float alpha = 1.0F;
        int materialIndex = -1;
    };

    SDL_Window* mWindow = nullptr;
    bool mOwnWindow = false;
    bool mOwnSdl = false;

    bool mFrameBegun = false;
    int mWidth = 0;
    int mHeight = 0;
    int mOutputWidth = 0;
    int mOutputHeight = 0;
    std::uint32_t mClearColor = 0x000000FFU;
    bool mDebugOverlayEnabled = false;
    std::uint64_t mFrameCpuHash = 1469598103934665603ULL;
    std::uint64_t mLastInternalCpuHash = 0ULL;
    std::uint64_t mLastPresentedCpuHash = 0ULL;
    std::size_t mCapturedLayerCount = 0U;
    std::array<std::uint64_t, kMaxCaptureLayers> mFrameLayerCpuHashes{};
    std::array<std::uint64_t, kMaxCaptureLayers> mLastLayerCpuHashes{};
    std::vector<DebugPaneRecord> mDebugPaneRecords;
    std::vector<DebugPaneRecord> mLastDebugPaneRecords;

    struct Impl;
    Impl* mImpl = nullptr;
};

}  // namespace pcport
