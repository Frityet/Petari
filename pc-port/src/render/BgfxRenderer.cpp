#include "render/BgfxRenderer.hpp"

#include "core/Logger.hpp"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif

#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if __has_include(<stb/stb_image_write.h>)
#include <stb/stb_image_write.h>
#elif __has_include(<stb_image_write.h>)
#include <stb_image_write.h>
#else
#error "stb_image_write.h is required for PNG export"
#endif

namespace pcport {
namespace {

constexpr std::uint8_t kViewLayoutComposite = 0U;
constexpr std::uint8_t kViewCompositePresented = 1U;
constexpr std::uint8_t kViewSwapchainPresent = 2U;
constexpr std::uint8_t kViewLayerBase = 8U;
constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ULL;

float AnchorOffsetX(std::uint8_t basePosition, float width) {
    switch (basePosition % 3U) {
    case 0:
        return 0.0F;
    case 1:
        return -width * 0.5F;
    case 2:
        return -width;
    default:
        return 0.0F;
    }
}

float AnchorOffsetY(std::uint8_t basePosition, float height) {
    const std::uint8_t row = static_cast<std::uint8_t>((basePosition / 3U) % 3U);
    switch (row) {
    case 0:
        return 0.0F;
    case 1:
        return -height * 0.5F;
    case 2:
        return -height;
    default:
        return 0.0F;
    }
}

std::uint8_t ClampU8(float value) {
    if (value <= 0.0F) {
        return 0U;
    }
    if (value >= 255.0F) {
        return 255U;
    }
    return static_cast<std::uint8_t>(std::lround(value));
}

std::uint32_t PackAbgr(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    return (static_cast<std::uint32_t>(a) << 24U) | (static_cast<std::uint32_t>(b) << 16U) | (static_cast<std::uint32_t>(g) << 8U) |
           static_cast<std::uint32_t>(r);
}

std::uint64_t HashBytes(const std::vector<std::uint8_t>& bytes) {
    std::uint64_t hash = kFnvOffsetBasis;
    for (const std::uint8_t byte : bytes) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

void HashAppend(std::uint64_t* hash, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        *hash ^= static_cast<std::uint64_t>(bytes[i]);
        *hash *= 1099511628211ULL;
    }
}

template <typename T>
void HashAppendPod(std::uint64_t* hash, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    HashAppend(hash, &value, sizeof(T));
}

void HashAppendFloat(std::uint64_t* hash, float value) {
    std::uint32_t bits = 0U;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    HashAppendPod(hash, bits);
}

void HashAppendString(std::uint64_t* hash, const std::string& value) {
    HashAppend(hash, value.data(), value.size());
}

std::uint32_t HashStringFnv1a(std::string_view text) {
    std::uint32_t hash = 2166136261U;
    for (const char ch : text) {
        hash ^= static_cast<std::uint8_t>(ch);
        hash *= 16777619U;
    }
    return hash;
}

std::uint32_t DebugColorFromPaneName(std::string_view paneName) {
    const std::uint32_t hash = HashStringFnv1a(paneName);
    const std::uint8_t r = static_cast<std::uint8_t>(64U + ((hash >> 0U) & 0x7FU));
    const std::uint8_t g = static_cast<std::uint8_t>(64U + ((hash >> 8U) & 0x7FU));
    const std::uint8_t b = static_cast<std::uint8_t>(64U + ((hash >> 16U) & 0x7FU));
    return PackAbgr(r, g, b, 180U);
}

std::string EscapeCsv(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size() + 2U);
    escaped.push_back('"');
    for (const char ch : text) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

bool WritePpmRgb(const std::filesystem::path& path, const std::uint8_t* pixels, int width, int height, std::uint32_t pitchBytes, bool bgra,
                 bool yFlip) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file << "P6\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; ++y) {
        const int srcY = yFlip ? (height - 1 - y) : y;
        const std::size_t rowStart = static_cast<std::size_t>(srcY) * pitchBytes;
        for (int x = 0; x < width; ++x) {
            const std::size_t index = rowStart + static_cast<std::size_t>(x) * 4U;
            const std::uint8_t r = bgra ? pixels[index + 2U] : pixels[index + 0U];
            const std::uint8_t g = pixels[index + 1U];
            const std::uint8_t b = bgra ? pixels[index + 0U] : pixels[index + 2U];
            file.put(static_cast<char>(r));
            file.put(static_cast<char>(g));
            file.put(static_cast<char>(b));
        }
    }

    return true;
}

std::vector<std::uint8_t> ConvertToRgba(const std::uint8_t* pixels, int width, int height, std::uint32_t pitchBytes, bool bgra, bool yFlip) {
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0U);
    for (int y = 0; y < height; ++y) {
        const int srcY = yFlip ? (height - 1 - y) : y;
        const std::size_t rowStart = static_cast<std::size_t>(srcY) * pitchBytes;
        for (int x = 0; x < width; ++x) {
            const std::size_t src = rowStart + static_cast<std::size_t>(x) * 4U;
            const std::size_t dst = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4U;
            rgba[dst + 0U] = bgra ? pixels[src + 2U] : pixels[src + 0U];
            rgba[dst + 1U] = pixels[src + 1U];
            rgba[dst + 2U] = bgra ? pixels[src + 0U] : pixels[src + 2U];
            rgba[dst + 3U] = pixels[src + 3U];
        }
    }
    return rgba;
}

bool WritePngRgba(const std::filesystem::path& path, const std::uint8_t* pixels, int width, int height, std::uint32_t pitchBytes, bool bgra,
                  bool yFlip) {
    const std::vector<std::uint8_t> rgba = ConvertToRgba(pixels, width, height, pitchBytes, bgra, yFlip);
    const int wrote = stbi_write_png(path.string().c_str(), width, height, 4, rgba.data(), width * 4);
    return wrote != 0;
}

std::filesystem::path GetExecutableDir() {
    std::array<char, 4096> buffer{};
    const ssize_t bytes = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1U);
    if (bytes <= 0) {
        return std::filesystem::current_path();
    }
    buffer[static_cast<std::size_t>(bytes)] = '\0';
    return std::filesystem::path(buffer.data()).parent_path();
}

bool HasShaderBinary(const std::filesystem::path& directory, std::string_view name) {
    const std::filesystem::path filePath = directory / std::string(name);
    return std::filesystem::is_regular_file(filePath);
}

bool IsShaderDirectory(const std::filesystem::path& directory) {
    return HasShaderBinary(directory, "vs_layout.bin") && HasShaderBinary(directory, "fs_layout.bin");
}

std::filesystem::path ResolveShaderDirectory() {
    if (const char* shaderDirEnv = std::getenv("PCPORT_BGFX_SHADER_DIR"); shaderDirEnv != nullptr && shaderDirEnv[0] != '\0') {
        const std::filesystem::path shaderDir = shaderDirEnv;
        if (IsShaderDirectory(shaderDir)) {
            return shaderDir;
        }
    }

    const std::filesystem::path executableDir = GetExecutableDir();
    const std::filesystem::path currentDir = std::filesystem::current_path();
    const std::array<std::filesystem::path, 8> candidates = {
        executableDir / "shaders" / "glsl",
        currentDir / "shaders" / "glsl",
        currentDir / "build" / "linux" / "x86_64" / "debug" / "shaders" / "glsl",
        currentDir / "build" / "linux" / "x86_64" / "release" / "shaders" / "glsl",
        currentDir / "pc-port" / "build" / "linux" / "x86_64" / "debug" / "shaders" / "glsl",
        currentDir / "pc-port" / "build" / "linux" / "x86_64" / "release" / "shaders" / "glsl",
        currentDir.parent_path() / "build" / "linux" / "x86_64" / "debug" / "shaders" / "glsl",
        currentDir.parent_path() / "build" / "linux" / "x86_64" / "release" / "shaders" / "glsl",
    };

    for (const std::filesystem::path& candidate : candidates) {
        if (IsShaderDirectory(candidate)) {
            return candidate;
        }
    }

    throw std::runtime_error("Failed to locate bgfx shaders. Set PCPORT_BGFX_SHADER_DIR to a directory containing vs_layout.bin and fs_layout.bin.");
}

std::vector<std::uint8_t> ReadFileBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader: " + path.string());
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

bgfx::PlatformData GetPlatformData(SDL_Window* window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        throw std::runtime_error(std::string("SDL_GetWindowWMInfo failed: ") + SDL_GetError());
    }

    bgfx::PlatformData platformData{};

#if defined(SDL_VIDEO_DRIVER_X11)
    if (wmInfo.subsystem == SDL_SYSWM_X11) {
        platformData.ndt = wmInfo.info.x11.display;
        platformData.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(wmInfo.info.x11.window));
        return platformData;
    }
#endif

#if defined(SDL_VIDEO_DRIVER_WAYLAND)
    if (wmInfo.subsystem == SDL_SYSWM_WAYLAND) {
        platformData.ndt = wmInfo.info.wl.display;
        platformData.nwh = wmInfo.info.wl.surface;
        return platformData;
    }
#endif

    throw std::runtime_error("Unsupported SDL window subsystem for bgfx");
}

struct BgfxVertex {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    std::uint32_t abgr = 0U;
    float u = 0.0F;
    float v = 0.0F;

    static bgfx::VertexLayout sLayout;

    static void InitLayout() {
        sLayout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();
    }
};

bgfx::VertexLayout BgfxVertex::sLayout;

void SubmitQuad(std::uint8_t viewId, bgfx::ProgramHandle program, bgfx::UniformHandle sampler, bgfx::TextureHandle texture, float x0, float y0,
                float x1, float y1, float u0, float v0, float u1, float v1, std::uint32_t colorTl, std::uint32_t colorTr,
                std::uint32_t colorBl, std::uint32_t colorBr, std::uint64_t state) {
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    constexpr std::uint32_t kNumVertices = 4U;
    constexpr std::uint32_t kNumIndices = 6U;
    if (bgfx::getAvailTransientVertexBuffer(kNumVertices, BgfxVertex::sLayout) < kNumVertices ||
        bgfx::getAvailTransientIndexBuffer(kNumIndices) < kNumIndices) {
        return;
    }
    bgfx::allocTransientVertexBuffer(&tvb, kNumVertices, BgfxVertex::sLayout);
    bgfx::allocTransientIndexBuffer(&tib, kNumIndices);

    auto* vertices = reinterpret_cast<BgfxVertex*>(tvb.data);
    vertices[0] = BgfxVertex{x0, y0, 0.0F, colorTl, u0, v0};
    vertices[1] = BgfxVertex{x1, y0, 0.0F, colorTr, u1, v0};
    vertices[2] = BgfxVertex{x0, y1, 0.0F, colorBl, u0, v1};
    vertices[3] = BgfxVertex{x1, y1, 0.0F, colorBr, u1, v1};

    auto* indices = reinterpret_cast<std::uint16_t*>(tib.data);
    indices[0] = 0U;
    indices[1] = 1U;
    indices[2] = 2U;
    indices[3] = 1U;
    indices[4] = 3U;
    indices[5] = 2U;

    bgfx::setTexture(0U, sampler, texture);
    bgfx::setState(state);
    bgfx::setVertexBuffer(0U, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::submit(viewId, program);
}

void SubmitSolidRect(std::uint8_t viewId, bgfx::ProgramHandle program, bgfx::UniformHandle sampler, bgfx::TextureHandle texture, float x0, float y0,
                     float x1, float y1, std::uint32_t abgrColor, std::uint64_t state) {
    SubmitQuad(viewId, program, sampler, texture, x0, y0, x1, y1, 0.0F, 0.0F, 1.0F, 1.0F, abgrColor, abgrColor, abgrColor, abgrColor, state);
}

class BgfxCallbacks final : public bgfx::CallbackI {
public:
    void BeginScreenshotRequest(std::string token) {
        std::scoped_lock lock(mMutex);
        mPendingToken = std::move(token);
        mScreenshotReady = false;
        mScreenshotPixels.clear();
        mScreenshotWidth = 0U;
        mScreenshotHeight = 0U;
        mScreenshotPitch = 0U;
        mScreenshotYFlip = false;
    }

    bool ConsumeScreenshot(const std::string& token, std::vector<std::uint8_t>* pixels, std::uint32_t* width, std::uint32_t* height,
                           std::uint32_t* pitch, bool* yflip) {
        std::scoped_lock lock(mMutex);
        if (!mScreenshotReady || token != mPendingToken) {
            return false;
        }

        *pixels = std::move(mScreenshotPixels);
        *width = mScreenshotWidth;
        *height = mScreenshotHeight;
        *pitch = mScreenshotPitch;
        *yflip = mScreenshotYFlip;
        mScreenshotReady = false;
        return true;
    }

    void fatal(const char* filePath, std::uint16_t line, bgfx::Fatal::Enum code, const char* str) override {
        std::fprintf(stderr, "[bgfx][fatal] %s:%u code=%d %s\n", filePath != nullptr ? filePath : "<unknown>", static_cast<unsigned>(line),
                     static_cast<int>(code), str != nullptr ? str : "<no message>");
        std::abort();
    }

    void traceVargs(const char* /*filePath*/, std::uint16_t /*line*/, const char* format, va_list argList) override {
        std::vfprintf(stderr, format, argList);
    }

    void profilerBegin(const char* /*name*/, std::uint32_t /*abgr*/, const char* /*filePath*/, std::uint16_t /*line*/) override {}
    void profilerBeginLiteral(const char* /*name*/, std::uint32_t /*abgr*/, const char* /*filePath*/, std::uint16_t /*line*/) override {}
    void profilerEnd() override {}
    std::uint32_t cacheReadSize(std::uint64_t /*id*/) override { return 0U; }
    bool cacheRead(std::uint64_t /*id*/, void* /*data*/, std::uint32_t /*size*/) override { return false; }
    void cacheWrite(std::uint64_t /*id*/, const void* /*data*/, std::uint32_t /*size*/) override {}

    void screenShot(const char* filePath, std::uint32_t width, std::uint32_t height, std::uint32_t pitch, const void* data, std::uint32_t size,
                    bool yflip) override {
        const std::string requestedPath = (filePath != nullptr) ? filePath : "";

        std::scoped_lock lock(mMutex);
        if (requestedPath != mPendingToken || data == nullptr || size == 0U) {
            return;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(data);
        mScreenshotPixels.assign(bytes, bytes + size);
        mScreenshotWidth = width;
        mScreenshotHeight = height;
        mScreenshotPitch = pitch;
        mScreenshotYFlip = yflip;
        mScreenshotReady = true;
    }

    void captureBegin(std::uint32_t /*width*/, std::uint32_t /*height*/, std::uint32_t /*pitch*/, bgfx::TextureFormat::Enum /*format*/,
                      bool /*yflip*/) override {}
    void captureEnd() override {}
    void captureFrame(const void* /*data*/, std::uint32_t /*size*/) override {}

private:
    std::mutex mMutex;
    std::string mPendingToken;
    bool mScreenshotReady = false;
    std::uint32_t mScreenshotWidth = 0U;
    std::uint32_t mScreenshotHeight = 0U;
    std::uint32_t mScreenshotPitch = 0U;
    bool mScreenshotYFlip = false;
    std::vector<std::uint8_t> mScreenshotPixels;
};

}  // namespace

struct BgfxRenderer::Impl {
    Impl() {
        layerColors.fill(BGFX_INVALID_HANDLE);
        layerFramebuffers.fill(BGFX_INVALID_HANDLE);
    }

    BgfxCallbacks callbacks;

    bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;

    bgfx::TextureHandle whiteTexture = BGFX_INVALID_HANDLE;

    bgfx::TextureHandle internalColor = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle internalFramebuffer = BGFX_INVALID_HANDLE;

    bgfx::TextureHandle presentedColor = BGFX_INVALID_HANDLE;
    bgfx::FrameBufferHandle presentedFramebuffer = BGFX_INVALID_HANDLE;
    std::array<bgfx::TextureHandle, BgfxRenderer::kMaxCaptureLayers> layerColors{};
    std::array<bgfx::FrameBufferHandle, BgfxRenderer::kMaxCaptureLayers> layerFramebuffers{};

    std::unordered_map<const ImageRGBA*, bgfx::TextureHandle> textures;
};

BgfxRenderer::BgfxRenderer(int width, int height) {
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0U) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }
        mOwnSdl = true;
    }

    mWindow = SDL_CreateWindow("pc-port-headless", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width * 2, height * 2,
                               SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (mWindow == nullptr) {
        if (mOwnSdl) {
            SDL_Quit();
            mOwnSdl = false;
        }
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    mOwnWindow = true;
    Initialize(width, height);
}

BgfxRenderer::BgfxRenderer(SDL_Window* window, int width, int height) {
    if (window == nullptr) {
        throw std::runtime_error("BgfxRenderer requires a valid SDL_Window");
    }

    mWindow = window;
    Initialize(width, height);
}

BgfxRenderer::~BgfxRenderer() {
    Shutdown();

    if (mOwnWindow && mWindow != nullptr) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }

    if (mOwnSdl) {
        SDL_Quit();
        mOwnSdl = false;
    }
}

void BgfxRenderer::Initialize(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Renderer dimensions must be positive");
    }

    mImpl = new Impl();

    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetWindowSize(mWindow, &outputWidth, &outputHeight);
    outputWidth = std::max(outputWidth, 1);
    outputHeight = std::max(outputHeight, 1);

    const bgfx::PlatformData platformData = GetPlatformData(mWindow);
    bgfx::setPlatformData(platformData);

    // Run bgfx in single-thread mode to avoid driver instability with hidden SDL windows in tests/headless runs.
    bgfx::renderFrame();

    bgfx::Init init{};
    init.type = bgfx::RendererType::OpenGL;
    init.callback = &mImpl->callbacks;
    init.platformData = platformData;
    init.resolution.width = static_cast<std::uint32_t>(outputWidth);
    init.resolution.height = static_cast<std::uint32_t>(outputHeight);
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        delete mImpl;
        mImpl = nullptr;
        throw std::runtime_error("bgfx::init failed");
    }

    BgfxVertex::InitLayout();

    mImpl->sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    const std::filesystem::path shaderDir = ResolveShaderDirectory();

    const auto createShader = [&](const char* filename) {
        const std::vector<std::uint8_t> bytes = ReadFileBytes(shaderDir / filename);
        const bgfx::Memory* memory = bgfx::copy(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
        return bgfx::createShader(memory);
    };

    const bgfx::ShaderHandle vs = createShader("vs_layout.bin");
    const bgfx::ShaderHandle fs = createShader("fs_layout.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) {
        throw std::runtime_error("Failed to create bgfx shader handles");
    }

    mImpl->program = bgfx::createProgram(vs, fs, true);

    const std::uint32_t whitePixel = 0xFFFFFFFFU;
    const bgfx::Memory* whiteMemory = bgfx::copy(&whitePixel, static_cast<std::uint32_t>(sizeof(whitePixel)));
    mImpl->whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::BGRA8, 0U, whiteMemory);

    Resize(width, height);
    EnsureOutputTargets(outputWidth, outputHeight);
}

void BgfxRenderer::Shutdown() {
    if (mImpl == nullptr) {
        return;
    }

    for (const auto& [_, texture] : mImpl->textures) {
        if (bgfx::isValid(texture)) {
            bgfx::destroy(texture);
        }
    }

    if (bgfx::isValid(mImpl->presentedFramebuffer)) {
        bgfx::destroy(mImpl->presentedFramebuffer);
    }
    if (bgfx::isValid(mImpl->presentedColor)) {
        bgfx::destroy(mImpl->presentedColor);
    }

    if (bgfx::isValid(mImpl->internalFramebuffer)) {
        bgfx::destroy(mImpl->internalFramebuffer);
    }
    if (bgfx::isValid(mImpl->internalColor)) {
        bgfx::destroy(mImpl->internalColor);
    }

    for (std::size_t i = 0; i < kMaxCaptureLayers; ++i) {
        if (bgfx::isValid(mImpl->layerFramebuffers[i])) {
            bgfx::destroy(mImpl->layerFramebuffers[i]);
        }
        if (bgfx::isValid(mImpl->layerColors[i])) {
            bgfx::destroy(mImpl->layerColors[i]);
        }
    }

    if (bgfx::isValid(mImpl->whiteTexture)) {
        bgfx::destroy(mImpl->whiteTexture);
    }

    if (bgfx::isValid(mImpl->program)) {
        bgfx::destroy(mImpl->program);
    }

    if (bgfx::isValid(mImpl->sampler)) {
        bgfx::destroy(mImpl->sampler);
    }

    bgfx::shutdown();

    delete mImpl;
    mImpl = nullptr;
}

int BgfxRenderer::GetWidth() const {
    return mWidth;
}

int BgfxRenderer::GetHeight() const {
    return mHeight;
}

void BgfxRenderer::SetDebugOverlayEnabled(bool enabled) {
    mDebugOverlayEnabled = enabled;
}

bool BgfxRenderer::GetDebugOverlayEnabled() const {
    return mDebugOverlayEnabled;
}

int BgfxRenderer::GetCapturedLayerCount() const {
    return static_cast<int>(mCapturedLayerCount);
}

void BgfxRenderer::Resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Renderer dimensions must be positive");
    }

    mWidth = width;
    mHeight = height;

    if (bgfx::isValid(mImpl->internalFramebuffer)) {
        bgfx::destroy(mImpl->internalFramebuffer);
        mImpl->internalFramebuffer = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(mImpl->internalColor)) {
        bgfx::destroy(mImpl->internalColor);
        mImpl->internalColor = BGFX_INVALID_HANDLE;
    }

    mImpl->internalColor =
        bgfx::createTexture2D(static_cast<std::uint16_t>(mWidth), static_cast<std::uint16_t>(mHeight), false, 1,
                              bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | BGFX_TEXTURE_READ_BACK);
    bgfx::TextureHandle internalTextures[] = {mImpl->internalColor};
    mImpl->internalFramebuffer = bgfx::createFrameBuffer(1, internalTextures, false);

    for (std::size_t i = 0; i < kMaxCaptureLayers; ++i) {
        if (bgfx::isValid(mImpl->layerFramebuffers[i])) {
            bgfx::destroy(mImpl->layerFramebuffers[i]);
            mImpl->layerFramebuffers[i] = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(mImpl->layerColors[i])) {
            bgfx::destroy(mImpl->layerColors[i]);
            mImpl->layerColors[i] = BGFX_INVALID_HANDLE;
        }

        mImpl->layerColors[i] =
            bgfx::createTexture2D(static_cast<std::uint16_t>(mWidth), static_cast<std::uint16_t>(mHeight), false, 1,
                                  bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT);
        if (!bgfx::isValid(mImpl->layerColors[i])) {
            continue;
        }

        bgfx::TextureHandle layerTextures[] = {mImpl->layerColors[i]};
        mImpl->layerFramebuffers[i] = bgfx::createFrameBuffer(1, layerTextures, false);
    }
}

void BgfxRenderer::EnsureOutputTargets(int width, int height) {
    width = std::max(width, 1);
    height = std::max(height, 1);

    if (width == mOutputWidth && height == mOutputHeight && bgfx::isValid(mImpl->presentedFramebuffer)) {
        return;
    }

    mOutputWidth = width;
    mOutputHeight = height;

    bgfx::reset(static_cast<std::uint32_t>(mOutputWidth), static_cast<std::uint32_t>(mOutputHeight), BGFX_RESET_VSYNC);

    if (bgfx::isValid(mImpl->presentedFramebuffer)) {
        bgfx::destroy(mImpl->presentedFramebuffer);
        mImpl->presentedFramebuffer = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(mImpl->presentedColor)) {
        bgfx::destroy(mImpl->presentedColor);
        mImpl->presentedColor = BGFX_INVALID_HANDLE;
    }

    mImpl->presentedColor =
        bgfx::createTexture2D(static_cast<std::uint16_t>(mOutputWidth), static_cast<std::uint16_t>(mOutputHeight), false, 1,
                              bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | BGFX_TEXTURE_READ_BACK);
    bgfx::TextureHandle textures[] = {mImpl->presentedColor};
    mImpl->presentedFramebuffer = bgfx::createFrameBuffer(1, textures, false);
}

void BgfxRenderer::BeginFrame() {
    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetWindowSize(mWindow, &outputWidth, &outputHeight);
    EnsureOutputTargets(outputWidth, outputHeight);

    float ortho[16];
    bx::mtxOrtho(ortho, 0.0F, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0F, 0.0F, 1000.0F, 0.0F,
                 bgfx::getCaps()->homogeneousDepth);

    bgfx::setViewName(kViewLayoutComposite, "layout_composite");
    bgfx::setViewRect(kViewLayoutComposite, 0, 0, static_cast<std::uint16_t>(mWidth), static_cast<std::uint16_t>(mHeight));
    bgfx::setViewFrameBuffer(kViewLayoutComposite, mImpl->internalFramebuffer);
    bgfx::setViewTransform(kViewLayoutComposite, nullptr, ortho);
    bgfx::setViewClear(kViewLayoutComposite, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, mClearColor, 1.0F, 0U);
    bgfx::touch(kViewLayoutComposite);

    mFrameCpuHash = kFnvOffsetBasis;
    HashAppendPod(&mFrameCpuHash, mWidth);
    HashAppendPod(&mFrameCpuHash, mHeight);
    HashAppendPod(&mFrameCpuHash, mOutputWidth);
    HashAppendPod(&mFrameCpuHash, mOutputHeight);
    HashAppendPod(&mFrameCpuHash, mClearColor);

    mCapturedLayerCount = 0U;
    for (std::size_t i = 0; i < kMaxCaptureLayers; ++i) {
        mFrameLayerCpuHashes[i] = kFnvOffsetBasis;
        HashAppendPod(&mFrameLayerCpuHashes[i], mWidth);
        HashAppendPod(&mFrameLayerCpuHashes[i], mHeight);
        HashAppendPod(&mFrameLayerCpuHashes[i], i);
    }

    mDebugPaneRecords.clear();

    mFrameBegun = true;
}

void BgfxRenderer::Clear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    mClearColor = PackAbgr(r, g, b, a);
}

void BgfxRenderer::RenderLayout(const BrlytLayout& layout) {
    const int rootIndex = layout.GetRootPaneIndex();
    if (rootIndex < 0) {
        return;
    }

    TransformContext context;
    context.originX = static_cast<float>(layout.GetWidth()) * 0.5F;
    context.originY = static_cast<float>(layout.GetHeight()) * 0.5F;

    RenderPaneRecursive(layout, rootIndex, context, kViewLayoutComposite, &mFrameCpuHash, true);

    const std::size_t layerIndex = mCapturedLayerCount;
    if (layerIndex < kMaxCaptureLayers && bgfx::isValid(mImpl->layerFramebuffers[layerIndex])) {
        const std::uint8_t layerView = static_cast<std::uint8_t>(kViewLayerBase + layerIndex);
        float orthoLayer[16];
        bx::mtxOrtho(orthoLayer, 0.0F, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0F, 0.0F, 1000.0F, 0.0F,
                     bgfx::getCaps()->homogeneousDepth);

        char viewName[32];
        std::snprintf(viewName, sizeof(viewName), "layout_layer_%zu", layerIndex);
        bgfx::setViewName(layerView, viewName);
        bgfx::setViewRect(layerView, 0, 0, static_cast<std::uint16_t>(mWidth), static_cast<std::uint16_t>(mHeight));
        bgfx::setViewFrameBuffer(layerView, mImpl->layerFramebuffers[layerIndex]);
        bgfx::setViewTransform(layerView, nullptr, orthoLayer);
        bgfx::setViewClear(layerView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, PackAbgr(0U, 0U, 0U, 0U), 1.0F, 0U);
        bgfx::touch(layerView);

        RenderPaneRecursive(layout, rootIndex, context, layerView, &mFrameLayerCpuHashes[layerIndex], false);
        ++mCapturedLayerCount;
    }
}

void BgfxRenderer::RenderPaneRecursive(const BrlytLayout& layout, int paneIndex, const TransformContext& parent, std::uint8_t viewId,
                                       std::uint64_t* hashAccumulator, bool collectDebugPaneRecords) {
    const auto& panes = layout.GetPanes();
    if (paneIndex < 0 || paneIndex >= static_cast<int>(panes.size())) {
        return;
    }

    const PaneResource& pane = panes[static_cast<std::size_t>(paneIndex)];
    if (!parent.visible || !pane.current.visible) {
        return;
    }

    const float worldScaleX = parent.scaleX * pane.current.sx;
    const float worldScaleY = parent.scaleY * pane.current.sy;

    const float paneOriginX = parent.originX + pane.current.tx * parent.scaleX;
    const float paneOriginY = parent.originY - pane.current.ty * parent.scaleY;

    const float drawX = paneOriginX + AnchorOffsetX(pane.basePosition, pane.current.width) * worldScaleX;
    const float drawY = paneOriginY + AnchorOffsetY(pane.basePosition, pane.current.height) * worldScaleY;
    const float drawW = pane.current.width * worldScaleX;
    const float drawH = pane.current.height * worldScaleY;

    const float alpha = parent.alpha * (static_cast<float>(pane.current.alpha) / 255.0F);

    switch (pane.type) {
    case PaneType::Picture:
    case PaneType::TextBox:
        DrawPaneQuad(pane, layout, drawX, drawY, drawW, drawH, alpha, viewId, hashAccumulator, collectDebugPaneRecords);
        break;
    case PaneType::Pane:
    case PaneType::Bounding:
        break;
    }

    TransformContext childContext;
    childContext.originX = paneOriginX;
    childContext.originY = paneOriginY;
    childContext.scaleX = worldScaleX;
    childContext.scaleY = worldScaleY;
    childContext.alpha = alpha;
    childContext.visible = true;

    for (const int child : pane.children) {
        RenderPaneRecursive(layout, child, childContext, viewId, hashAccumulator, collectDebugPaneRecords);
    }
}

void BgfxRenderer::DrawPaneQuad(const PaneResource& pane, const BrlytLayout& layout, float drawX, float drawY, float drawW, float drawH,
                                float alpha, std::uint8_t viewId, std::uint64_t* hashAccumulator, bool collectDebugPaneRecords) {
    if (!mFrameBegun || std::fabs(drawW) < 0.00001F || std::fabs(drawH) < 0.00001F || alpha <= 0.0F) {
        return;
    }

    const MaterialResource* material = layout.GetMaterial(pane.materialIndex);

    bgfx::TextureHandle textureHandle = mImpl->whiteTexture;
    if (material != nullptr && !material->textureIndices.empty()) {
        const ImageRGBA* image = layout.GetTexture(material->textureIndices[0]);
        if (image != nullptr && !image->Empty()) {
            const auto it = mImpl->textures.find(image);
            if (it != mImpl->textures.end()) {
                textureHandle = it->second;
            } else {
                const bgfx::Memory* memory =
                    bgfx::copy(image->pixels.data(), static_cast<std::uint32_t>(image->pixels.size() * sizeof(std::uint8_t)));
                bgfx::TextureHandle created =
                    bgfx::createTexture2D(static_cast<std::uint16_t>(image->width), static_cast<std::uint16_t>(image->height), false, 1,
                                          bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, memory);
                if (bgfx::isValid(created)) {
                    mImpl->textures.emplace(image, created);
                    textureHandle = created;
                }
            }
        }
    }

    const float x0 = drawX;
    const float x1 = drawX + drawW;
    const float y0 = drawY;
    const float y1 = drawY + drawH;

    const std::array<float, 8> uv = {
        pane.current.texOffsetU + pane.uv[0] * pane.current.texScaleU,
        pane.current.texOffsetV + pane.uv[1] * pane.current.texScaleV,
        pane.current.texOffsetU + pane.uv[2] * pane.current.texScaleU,
        pane.current.texOffsetV + pane.uv[3] * pane.current.texScaleV,
        pane.current.texOffsetU + pane.uv[4] * pane.current.texScaleU,
        pane.current.texOffsetV + pane.uv[5] * pane.current.texScaleV,
        pane.current.texOffsetU + pane.uv[6] * pane.current.texScaleU,
        pane.current.texOffsetV + pane.uv[7] * pane.current.texScaleV,
    };

    const float matR = (material != nullptr) ? (static_cast<float>(material->matColor[0]) / 255.0F) : 1.0F;
    const float matG = (material != nullptr) ? (static_cast<float>(material->matColor[1]) / 255.0F) : 1.0F;
    const float matB = (material != nullptr) ? (static_cast<float>(material->matColor[2]) / 255.0F) : 1.0F;
    const float matA = (material != nullptr) ? (static_cast<float>(material->matColor[3]) / 255.0F) : 1.0F;

    const auto colorForVertex = [&](std::size_t vertexIndex) -> std::uint32_t {
        const std::size_t base = vertexIndex * 4U;
        const float r = (static_cast<float>(pane.current.vertexColor[base + 0U]) / 255.0F) * matR;
        const float g = (static_cast<float>(pane.current.vertexColor[base + 1U]) / 255.0F) * matG;
        const float b = (static_cast<float>(pane.current.vertexColor[base + 2U]) / 255.0F) * matB;
        const float a = (static_cast<float>(pane.current.vertexColor[base + 3U]) / 255.0F) * matA * alpha;
        return PackAbgr(ClampU8(r * 255.0F), ClampU8(g * 255.0F), ClampU8(b * 255.0F), ClampU8(a * 255.0F));
    };

    const std::uint32_t color0 = colorForVertex(0U);
    const std::uint32_t color1 = colorForVertex(1U);
    const std::uint32_t color2 = colorForVertex(2U);
    const std::uint32_t color3 = colorForVertex(3U);

    if (collectDebugPaneRecords) {
        DebugPaneRecord record;
        record.name = pane.name;
        record.x = x0;
        record.y = y0;
        record.width = drawW;
        record.height = drawH;
        record.alpha = alpha;
        record.materialIndex = pane.materialIndex;
        mDebugPaneRecords.push_back(std::move(record));
    }

    if (hashAccumulator != nullptr) {
        HashAppendString(hashAccumulator, pane.name);
        HashAppendPod(hashAccumulator, pane.basePosition);
        HashAppendPod(hashAccumulator, pane.materialIndex);
        HashAppendFloat(hashAccumulator, x0);
        HashAppendFloat(hashAccumulator, y0);
        HashAppendFloat(hashAccumulator, x1);
        HashAppendFloat(hashAccumulator, y1);
        HashAppendFloat(hashAccumulator, alpha);
        for (float uvValue : uv) {
            HashAppendFloat(hashAccumulator, uvValue);
        }
        HashAppendPod(hashAccumulator, color0);
        HashAppendPod(hashAccumulator, color1);
        HashAppendPod(hashAccumulator, color2);
        HashAppendPod(hashAccumulator, color3);
        if (material != nullptr) {
            HashAppendString(hashAccumulator, material->name);
        }
    }

    SubmitQuad(viewId, mImpl->program, mImpl->sampler, textureHandle, x0, y0, x1, y1, uv[0], uv[1], uv[6], uv[7], color0, color1, color2, color3,
               BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA);

    if (mDebugOverlayEnabled) {
        DrawDebugOutline(x0, y0, x1, y1, DebugColorFromPaneName(pane.name), viewId);
    }
}

void BgfxRenderer::DrawDebugOutline(float x0, float y0, float x1, float y1, std::uint32_t abgrColor, std::uint8_t viewId) {
    if (!mFrameBegun || !bgfx::isValid(mImpl->whiteTexture)) {
        return;
    }

    const float left = std::min(x0, x1);
    const float right = std::max(x0, x1);
    const float top = std::min(y0, y1);
    const float bottom = std::max(y0, y1);
    const float width = right - left;
    const float height = bottom - top;

    if (width <= 0.0F || height <= 0.0F) {
        return;
    }

    const float thickness = std::clamp(std::min(width, height) * 0.03F, 1.0F, 4.0F);
    constexpr std::uint64_t kState =
        BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_DEPTH_TEST_ALWAYS | BGFX_STATE_MSAA;

    SubmitSolidRect(viewId, mImpl->program, mImpl->sampler, mImpl->whiteTexture, left, top, right, top + thickness, abgrColor, kState);
    SubmitSolidRect(viewId, mImpl->program, mImpl->sampler, mImpl->whiteTexture, left, bottom - thickness, right, bottom, abgrColor, kState);
    SubmitSolidRect(viewId, mImpl->program, mImpl->sampler, mImpl->whiteTexture, left, top + thickness, left + thickness,
                    bottom - thickness, abgrColor, kState);
    SubmitSolidRect(viewId, mImpl->program, mImpl->sampler, mImpl->whiteTexture, right - thickness, top + thickness, right, bottom - thickness,
                    abgrColor, kState);
}

void BgfxRenderer::EndFrame() {
    if (!mFrameBegun) {
        return;
    }

    float orthoPresented[16];
    bx::mtxOrtho(orthoPresented, 0.0F, static_cast<float>(mOutputWidth), static_cast<float>(mOutputHeight), 0.0F, 0.0F, 1000.0F, 0.0F,
                 bgfx::getCaps()->homogeneousDepth);

    bgfx::setViewName(kViewCompositePresented, "scale_to_presented");
    bgfx::setViewRect(kViewCompositePresented, 0, 0, static_cast<std::uint16_t>(mOutputWidth), static_cast<std::uint16_t>(mOutputHeight));
    bgfx::setViewFrameBuffer(kViewCompositePresented, mImpl->presentedFramebuffer);
    bgfx::setViewTransform(kViewCompositePresented, nullptr, orthoPresented);
    bgfx::setViewClear(kViewCompositePresented, BGFX_CLEAR_COLOR, PackAbgr(0U, 0U, 0U, 255U), 1.0F, 0U);
    bgfx::touch(kViewCompositePresented);

    SubmitQuad(kViewCompositePresented, mImpl->program, mImpl->sampler, mImpl->internalColor, 0.0F, 0.0F, static_cast<float>(mOutputWidth),
               static_cast<float>(mOutputHeight), 0.0F, 0.0F, 1.0F, 1.0F, PackAbgr(255U, 255U, 255U, 255U),
               PackAbgr(255U, 255U, 255U, 255U), PackAbgr(255U, 255U, 255U, 255U), PackAbgr(255U, 255U, 255U, 255U),
               BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA);

    bgfx::setViewName(kViewSwapchainPresent, "present");
    bgfx::setViewRect(kViewSwapchainPresent, 0, 0, static_cast<std::uint16_t>(mOutputWidth), static_cast<std::uint16_t>(mOutputHeight));
    bgfx::setViewFrameBuffer(kViewSwapchainPresent, BGFX_INVALID_HANDLE);
    bgfx::setViewTransform(kViewSwapchainPresent, nullptr, orthoPresented);
    bgfx::touch(kViewSwapchainPresent);

    SubmitQuad(kViewSwapchainPresent, mImpl->program, mImpl->sampler, mImpl->presentedColor, 0.0F, 0.0F, static_cast<float>(mOutputWidth),
               static_cast<float>(mOutputHeight), 0.0F, 0.0F, 1.0F, 1.0F, PackAbgr(255U, 255U, 255U, 255U),
               PackAbgr(255U, 255U, 255U, 255U), PackAbgr(255U, 255U, 255U, 255U), PackAbgr(255U, 255U, 255U, 255U),
               BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA);

    mLastInternalCpuHash = mFrameCpuHash;
    mLastPresentedCpuHash = mFrameCpuHash;
    HashAppendPod(&mLastPresentedCpuHash, mOutputWidth);
    HashAppendPod(&mLastPresentedCpuHash, mOutputHeight);
    for (std::size_t i = 0; i < kMaxCaptureLayers; ++i) {
        mLastLayerCpuHashes[i] = (i < mCapturedLayerCount) ? mFrameLayerCpuHashes[i] : 0ULL;
    }
    mLastDebugPaneRecords = mDebugPaneRecords;

    bgfx::frame();
    mFrameBegun = false;
}

bool ReadBackTexture(bgfx::TextureHandle texture, int width, int height, std::vector<std::uint8_t>* pixels) {
    if (!bgfx::isValid(texture) || width <= 0 || height <= 0) {
        return false;
    }

    const bgfx::Caps* caps = bgfx::getCaps();
    if (caps == nullptr || (caps->supported & BGFX_CAPS_TEXTURE_READ_BACK) == 0ULL) {
        return false;
    }

    pixels->assign(static_cast<std::size_t>(width * height * 4), 0U);
    const std::uint32_t readFrame = bgfx::readTexture(texture, pixels->data());

    std::uint32_t frame = bgfx::frame();
    while (frame < readFrame) {
        frame = bgfx::frame();
    }
    return true;
}

bool BgfxRenderer::ReadSurface(CaptureSurface surface, std::vector<std::uint8_t>* pixels, int* width, int* height) {
    if (mFrameBegun) {
        return false;
    }

    bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
    int surfaceWidth = 0;
    int surfaceHeight = 0;

    if (surface == CaptureSurface::Internal) {
        texture = mImpl->internalColor;
        surfaceWidth = mWidth;
        surfaceHeight = mHeight;
    } else {
        texture = mImpl->presentedColor;
        surfaceWidth = mOutputWidth;
        surfaceHeight = mOutputHeight;
    }

    if (!ReadBackTexture(texture, surfaceWidth, surfaceHeight, pixels)) {
        return false;
    }

    *width = surfaceWidth;
    *height = surfaceHeight;
    return true;
}

bool BgfxRenderer::ReadLayerSurface(int layerIndex, std::vector<std::uint8_t>* pixels, int* width, int* height) {
    if (mFrameBegun || layerIndex < 0 || layerIndex >= static_cast<int>(mCapturedLayerCount) ||
        layerIndex >= static_cast<int>(kMaxCaptureLayers)) {
        return false;
    }

    const bgfx::TextureHandle texture = mImpl->layerColors[static_cast<std::size_t>(layerIndex)];
    if (!ReadBackTexture(texture, mWidth, mHeight, pixels)) {
        return false;
    }

    *width = mWidth;
    *height = mHeight;
    return true;
}

std::uint64_t BgfxRenderer::ComputeHash(CaptureSurface surface) {
    const std::uint64_t cpuHash = (surface == CaptureSurface::Internal) ? mLastInternalCpuHash : mLastPresentedCpuHash;
    if (cpuHash != 0ULL) {
        return cpuHash;
    }

    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!ReadSurface(surface, &pixels, &width, &height)) {
        return 0ULL;
    }
    return HashBytes(pixels);
}

std::uint64_t BgfxRenderer::ComputeLayerHash(int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(mCapturedLayerCount) || layerIndex >= static_cast<int>(kMaxCaptureLayers)) {
        return 0ULL;
    }

    const std::uint64_t cpuHash = mLastLayerCpuHashes[static_cast<std::size_t>(layerIndex)];
    if (cpuHash != 0ULL) {
        return cpuHash;
    }

    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!ReadLayerSurface(layerIndex, &pixels, &width, &height)) {
        return 0ULL;
    }
    return HashBytes(pixels);
}

bool BgfxRenderer::SaveDebugPaneList(const std::filesystem::path& path) const {
    std::ofstream file(path);
    if (!file) {
        return false;
    }

    file << "pane_index,pane_name,x,y,width,height,alpha,material_index,overlay_enabled\n";
    for (std::size_t i = 0; i < mLastDebugPaneRecords.size(); ++i) {
        const DebugPaneRecord& record = mLastDebugPaneRecords[i];
        file << i << ',' << EscapeCsv(record.name) << ',' << record.x << ',' << record.y << ',' << record.width << ',' << record.height << ','
             << record.alpha << ',' << record.materialIndex << ',' << (mDebugOverlayEnabled ? 1 : 0) << '\n';
    }
    return true;
}

bool BgfxRenderer::SavePpm(const std::filesystem::path& path, CaptureSurface surface) {
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!ReadSurface(surface, &pixels, &width, &height)) {
        bgfx::FrameBufferHandle screenshotSource = BGFX_INVALID_HANDLE;
        if (surface == CaptureSurface::Internal) {
            screenshotSource = mImpl->internalFramebuffer;
        } else {
            screenshotSource = mImpl->presentedFramebuffer;
        }

        for (int attempt = 0; attempt < 2; ++attempt) {
            if (attempt == 1 && (surface != CaptureSurface::Presented || !bgfx::isValid(screenshotSource))) {
                break;
            }
            if (attempt == 1) {
                screenshotSource = BGFX_INVALID_HANDLE;
            }

            const std::string token = path.string();
            mImpl->callbacks.BeginScreenshotRequest(token);
            bgfx::requestScreenShot(screenshotSource, token.c_str());

            std::vector<std::uint8_t> screenshotPixels;
            std::uint32_t screenshotWidth = 0U;
            std::uint32_t screenshotHeight = 0U;
            std::uint32_t screenshotPitch = 0U;
            bool screenshotYFlip = false;

            bool received = false;
            for (int i = 0; i < 6; ++i) {
                bgfx::frame();
                if (mImpl->callbacks.ConsumeScreenshot(token, &screenshotPixels, &screenshotWidth, &screenshotHeight, &screenshotPitch,
                                                       &screenshotYFlip)) {
                    received = true;
                    break;
                }
            }

            if (!received || screenshotWidth == 0U || screenshotHeight == 0U || screenshotPitch == 0U) {
                continue;
            }

            return WritePpmRgb(path, screenshotPixels.data(), static_cast<int>(screenshotWidth), static_cast<int>(screenshotHeight),
                               screenshotPitch, true, screenshotYFlip);
        }

        return false;
    }

    return WritePpmRgb(path, pixels.data(), width, height, static_cast<std::uint32_t>(width * 4), false, false);
}

bool BgfxRenderer::SavePng(const std::filesystem::path& path, CaptureSurface surface) {
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!ReadSurface(surface, &pixels, &width, &height)) {
        bgfx::FrameBufferHandle screenshotSource = BGFX_INVALID_HANDLE;
        if (surface == CaptureSurface::Internal) {
            screenshotSource = mImpl->internalFramebuffer;
        } else {
            screenshotSource = mImpl->presentedFramebuffer;
        }

        for (int attempt = 0; attempt < 2; ++attempt) {
            if (attempt == 1 && (surface != CaptureSurface::Presented || !bgfx::isValid(screenshotSource))) {
                break;
            }
            if (attempt == 1) {
                screenshotSource = BGFX_INVALID_HANDLE;
            }

            const std::string token = path.string();
            mImpl->callbacks.BeginScreenshotRequest(token);
            bgfx::requestScreenShot(screenshotSource, token.c_str());

            std::vector<std::uint8_t> screenshotPixels;
            std::uint32_t screenshotWidth = 0U;
            std::uint32_t screenshotHeight = 0U;
            std::uint32_t screenshotPitch = 0U;
            bool screenshotYFlip = false;

            bool received = false;
            for (int i = 0; i < 6; ++i) {
                bgfx::frame();
                if (mImpl->callbacks.ConsumeScreenshot(token, &screenshotPixels, &screenshotWidth, &screenshotHeight, &screenshotPitch,
                                                       &screenshotYFlip)) {
                    received = true;
                    break;
                }
            }

            if (!received || screenshotWidth == 0U || screenshotHeight == 0U || screenshotPitch == 0U) {
                continue;
            }

            return WritePngRgba(path, screenshotPixels.data(), static_cast<int>(screenshotWidth), static_cast<int>(screenshotHeight),
                                screenshotPitch, true, screenshotYFlip);
        }

        return false;
    }

    return WritePngRgba(path, pixels.data(), width, height, static_cast<std::uint32_t>(width * 4), false, false);
}

bool BgfxRenderer::SaveLayerPpm(const std::filesystem::path& path, int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(mCapturedLayerCount) || layerIndex >= static_cast<int>(kMaxCaptureLayers)) {
        return false;
    }

    const bgfx::FrameBufferHandle framebuffer = mImpl->layerFramebuffers[static_cast<std::size_t>(layerIndex)];
    if (!bgfx::isValid(framebuffer)) {
        return false;
    }

    const std::string token = path.string();
    mImpl->callbacks.BeginScreenshotRequest(token);
    bgfx::requestScreenShot(framebuffer, token.c_str());

    std::vector<std::uint8_t> screenshotPixels;
    std::uint32_t screenshotWidth = 0U;
    std::uint32_t screenshotHeight = 0U;
    std::uint32_t screenshotPitch = 0U;
    bool screenshotYFlip = false;

    bool received = false;
    for (int i = 0; i < 6; ++i) {
        bgfx::frame();
        if (mImpl->callbacks.ConsumeScreenshot(token, &screenshotPixels, &screenshotWidth, &screenshotHeight, &screenshotPitch,
                                               &screenshotYFlip)) {
            received = true;
            break;
        }
    }

    if (!received || screenshotWidth == 0U || screenshotHeight == 0U || screenshotPitch == 0U) {
        return false;
    }

    return WritePpmRgb(path, screenshotPixels.data(), static_cast<int>(screenshotWidth), static_cast<int>(screenshotHeight), screenshotPitch, true,
                       screenshotYFlip);
}

bool BgfxRenderer::SaveLayerPng(const std::filesystem::path& path, int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(mCapturedLayerCount) || layerIndex >= static_cast<int>(kMaxCaptureLayers)) {
        return false;
    }

    const bgfx::FrameBufferHandle framebuffer = mImpl->layerFramebuffers[static_cast<std::size_t>(layerIndex)];
    if (!bgfx::isValid(framebuffer)) {
        return false;
    }

    const std::string token = path.string();
    mImpl->callbacks.BeginScreenshotRequest(token);
    bgfx::requestScreenShot(framebuffer, token.c_str());

    std::vector<std::uint8_t> screenshotPixels;
    std::uint32_t screenshotWidth = 0U;
    std::uint32_t screenshotHeight = 0U;
    std::uint32_t screenshotPitch = 0U;
    bool screenshotYFlip = false;

    bool received = false;
    for (int i = 0; i < 6; ++i) {
        bgfx::frame();
        if (mImpl->callbacks.ConsumeScreenshot(token, &screenshotPixels, &screenshotWidth, &screenshotHeight, &screenshotPitch,
                                               &screenshotYFlip)) {
            received = true;
            break;
        }
    }

    if (!received || screenshotWidth == 0U || screenshotHeight == 0U || screenshotPitch == 0U) {
        return false;
    }

    return WritePngRgba(path, screenshotPixels.data(), static_cast<int>(screenshotWidth), static_cast<int>(screenshotHeight), screenshotPitch, true,
                        screenshotYFlip);
}

bool BgfxRenderer::SaveAllLayersPpm(const std::filesystem::path& directory, const std::string& baseName) {
    const bool internalOk = SavePpm(directory / (baseName + "_internal.ppm"), CaptureSurface::Internal);
    const bool presentedOk = SavePpm(directory / (baseName + "_presented.ppm"), CaptureSurface::Presented);
    if (!internalOk) {
        Log(LogLevel::Warn, LogCategory::App, "Internal PPM export unavailable for this backend/frame");
    }

    bool anyLayerSaved = false;
    for (int i = 0; i < GetCapturedLayerCount(); ++i) {
        anyLayerSaved = SaveLayerPpm(directory / (baseName + "_layer" + std::to_string(i) + ".ppm"), i) || anyLayerSaved;
    }

    if (GetCapturedLayerCount() > 0 && !anyLayerSaved) {
        Log(LogLevel::Warn, LogCategory::App, "Layer PPM export unavailable for this backend/frame");
    }

    return presentedOk;
}

bool BgfxRenderer::SaveAllLayersPng(const std::filesystem::path& directory, const std::string& baseName) {
    const bool internalOk = SavePng(directory / (baseName + "_internal.png"), CaptureSurface::Internal);
    const bool presentedOk = SavePng(directory / (baseName + "_presented.png"), CaptureSurface::Presented);
    if (!internalOk) {
        Log(LogLevel::Warn, LogCategory::App, "Internal PNG export unavailable for this backend/frame");
    }

    bool anyLayerSaved = false;
    for (int i = 0; i < GetCapturedLayerCount(); ++i) {
        anyLayerSaved = SaveLayerPng(directory / (baseName + "_layer" + std::to_string(i) + ".png"), i) || anyLayerSaved;
    }

    if (GetCapturedLayerCount() > 0 && !anyLayerSaved) {
        Log(LogLevel::Warn, LogCategory::App, "Layer PNG export unavailable for this backend/frame");
    }

    return presentedOk;
}

}  // namespace pcport
