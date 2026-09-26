#include "runtime/JasAudioDriver.hpp"
#include "JSystem/JAudio2/JASAiCtrl.hpp"
#include "JSystem/JAudio2/JASAramStream.hpp"
#include "JSystem/JAudio2/JASChannel.hpp"
#include "JSystem/JAudio2/JASCmdStack.hpp"
#include "JSystem/JAudio2/JASCriticalSection.hpp"
#include "JSystem/JAudio2/JASDSPChannel.hpp"
#include "JSystem/JAudio2/JASDSPInterface.hpp"
#include "JSystem/JAudio2/JASDriverIF.hpp"
#include "JSystem/JAudio2/JASDvdThread.hpp"
#include "JSystem/JAudio2/JASLfo.hpp"
#include "JSystem/JAudio2/JASTrack.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <aurora/allocation.hpp>
#include <aurora/audio_dma.hpp>
#include <aurora/endian.hpp>
#include <aurora/guest_thread.hpp>
#include <aurora/j_audio_dsp.hpp>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dolphin/ar.h>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace smgpc::audio {
    namespace {
        constexpr u32 ChannelAddress = 0x10000000;
        constexpr u32 EffectAddress = 0x20000000;
        constexpr u32 EffectBufferAddress = 0x30000000;
        constexpr u32 LeftAddress = 0x40000000;
        constexpr u32 RightAddress = 0x40001000;
        constexpr std::array<unsigned, 8> ChannelWord32Offsets{0x68, 0x70, 0x74, 0x10c, 0x110, 0x114, 0x118, 0x11c};

        class DspMemory final : public aurora::audio::JAudioDspMemory {
        public:
            std::array<u16, 64 * 0xc0> channels{};
            std::array<u16, 4 * 16> effects{};
            std::array<u16, 80> left{}, right{};

            std::span<u16> words(u32 address, std::size_t size) {
                auto region = [&](u32 base, std::span<u16> data) -> std::span<u16> {
                    if (address < base || (address - base) % 2)
                        return {};
                    const std::size_t offset = (address - base) / 2;
                    if (offset > data.size() || size > data.size() - offset)
                        return {};
                    return data.subspan(offset, size);
                };
                for (auto [base, data] : {std::pair{ChannelAddress, std::span<u16>(channels)},
                                          std::pair{EffectAddress, std::span<u16>(effects)},
                                          std::pair{LeftAddress, std::span<u16>(left)},
                                          std::pair{RightAddress, std::span<u16>(right)}}) {
                    auto result = region(base, data);
                    if (!result.empty())
                        return result;
                }
                for (u32 i = 0; i < 4; ++i) {
                    auto &fx = JASDsp::FX_BUF[i];
                    if (!fx._4)
                        continue;
                    auto result = region(EffectBufferAddress + i * 0x100000, {reinterpret_cast<u16 *>(fx._4), fx._2 * 80U});
                    if (!result.empty())
                        return result;
                }
                aurora::throw_host_exception<std::runtime_error>("JAudio DSP transfer outside a registered buffer");
            }
            void read_words(std::span<u16> dst, u32 address) override {
                const auto src = words(address, dst.size());
                std::copy(src.begin(), src.end(), dst.begin());
            }
            void write_words(u32 address, std::span<const u16> src) override {
                const auto dst = words(address, src.size());
                std::copy(src.begin(), src.end(), dst.begin());
            }
            std::span<const u8> sample_bytes(u32 address, std::size_t size) override {
                const auto extent = ARGetSize();
                if (address > extent || size > extent - address)
                    aurora::throw_host_exception<std::runtime_error>("JAudio DSP sample read outside ARAM");
                return {static_cast<const u8 *>(ARGetStorageAddress()) + address, size};
            }
            void receive_channels(unsigned batch) {
                static_assert(sizeof(JASDsp::TChannel) == 0x180);
                if (batch >= 4)
                    aurora::throw_host_exception<std::out_of_range>("Invalid DSP channel batch");
                std::memcpy(channels.data() + batch * 16 * 0xc0, JASDsp::CH_BUF + batch * 16, 16 * sizeof(JASDsp::TChannel));
                if constexpr (std::endian::native == std::endian::little) {
                    for (u32 i = batch * 16; i < (batch + 1) * 16; ++i)
                        for (auto offset : ChannelWord32Offsets)
                            std::swap(channels[i * 0xc0 + offset / 2], channels[i * 0xc0 + offset / 2 + 1]);
                }
            }
            void receive_parameters() {
                for (u32 i = 0; i < 4; ++i) {
                    const auto &fx = JASDsp::FX_BUF[i];
                    auto *out = effects.data() + i * 16;
                    const auto address = EffectBufferAddress + i * 0x100000;
                    out[0] = fx._0;
                    out[1] = fx._2;
                    out[2] = address >> 16;
                    out[3] = address;
                    out[4] = fx._8;
                    out[5] = fx._A;
                    out[6] = fx._C;
                    out[7] = fx._E;
                    std::copy_n(fx._10, 8, out + 8);
                }
            }
            void publish_status() {
                if constexpr (std::endian::native == std::endian::little) {
                    for (u32 i = 0; i < 64; ++i)
                        for (auto offset : ChannelWord32Offsets)
                            std::swap(channels[i * 0xc0 + offset / 2], channels[i * 0xc0 + offset / 2 + 1]);
                }
                for (u32 i = 0; i < 64; ++i)
                    std::memcpy(&JASDsp::CH_BUF[i], channels.data() + i * 0xc0, 0x100);
            }
        };

        struct Driver {
            DspMemory memory;
            aurora::audio::JAudioDspRenderer renderer{memory};
            aurora::audio::DmaAudioOutput output{static_cast<int>(JASDriver::getDacRate() + 0.5f)};
            std::vector<s16> dma = std::vector<s16>(JASDriver::getDacSize());
            unsigned dmaSubframes = 0;
            bool paused = false;
            std::atomic<bool> stopping = false;
            std::atomic<std::uint64_t> generation = 0;
            std::thread worker;
            std::mutex wakeMutex;
            std::condition_variable wake, frameReady;
            std::exception_ptr error;
#ifndef NDEBUG
            std::unique_ptr<FILE, decltype(&std::fclose)> pcmCapture{nullptr, &std::fclose};
            std::unique_ptr<FILE, decltype(&std::fclose)> channelCapture{nullptr, &std::fclose};
#endif
            Driver() {
#ifndef NDEBUG
                if (const char *prefix = std::getenv("SMGPC_JAS_CAPTURE"); prefix && *prefix) {
                    pcmCapture.reset(std::fopen((std::string(prefix) + ".pcm").c_str(), "wb"));
                    channelCapture.reset(std::fopen((std::string(prefix) + ".vpb").c_str(), "wb"));
                    if (!pcmCapture || !channelCapture)
                        aurora::throw_host_exception<std::runtime_error>("Cannot open JAudio diagnostic capture");
                }
                if (const char *mute = std::getenv("SMGPC_JAS_MUTE"); mute && std::string_view(mute) == "1")
                    output.set_gain(0.0f);
#endif
                renderer.SetFlags(2);  // JAudio's later DSP revision doubles Dolby gain.
                renderer.SetVPBBaseAddress(ChannelAddress);
                renderer.SetReverbPBBaseAddress(EffectAddress);
                std::array<s16, 256> resample{}, patterns{};
                std::array<s16, 128> sine{};
                const auto half = [](std::size_t i) -> s16 { return JASDsp::DSPRES_FILTER[i / 2] >> ((i & 1) ? 0 : 16); };
                for (unsigned i = 0; i < 256; ++i) {
                    resample[i] = half(i);
                    patterns[i] = half(256 + i);
                }
                for (unsigned i = 0; i < 128; ++i)
                    sine[i] = half(512 + i);
                renderer.SetResamplingCoeffs(std::move(resample));
                renderer.SetConstPatterns(std::move(patterns));
                renderer.SetSineTable(std::move(sine));
                std::array<s16, 32> afc{};
                for (unsigned i = 0; i < 32; ++i)
                    afc[i] = aurora::endian::read_big<s16>(JASDsp::DSPADPCM_FILTER + i * 2);
                renderer.SetAfcCoeffs(std::move(afc));
            }
        };
        std::unique_ptr<Driver> driver;
    }  // namespace

    void initialize_dsp() {
        if (driver)
            aurora::throw_host_exception<std::logic_error>("JAS DSP initialized twice");
        JASChannel::initBankDisposeMsgQueue();
        JASDsp::initBuffer();
        JASDSPChannel::initAll();
        JASChannel::newMemPool(0x48);
        const aurora::allocation::HostAllocationScope host;
        driver = std::make_unique<Driver>();
    }

    void upload_channel_batch(unsigned batch) {
        if (driver)
            driver->memory.receive_channels(batch);
    }

    void render_subframe() {
        if (!driver)
            return;
        JASCriticalSection critical;
        JASDriver::updateDSP();
        const aurora::allocation::HostAllocationScope host;
        driver->memory.receive_parameters();
        auto &renderer = driver->renderer;
        renderer.SetOutputVolume(static_cast<u16>(std::clamp(JASDsp::sDSPVolume * 4.0f * 4096.0f, 0.0f, 65535.0f)));
        renderer.SetOutputLeftBufferAddr(LeftAddress);
        renderer.SetOutputRightBufferAddr(RightAddress);
        renderer.PrepareFrame();
        for (u16 i = 0; i < 64; ++i)
            renderer.AddVoice(i);
        renderer.FinalizeFrame();
#ifndef NDEBUG
        if (driver->channelCapture)
            std::fwrite(driver->memory.channels.data(), sizeof(u16), driver->memory.channels.size(), driver->channelCapture.get());
#endif
        driver->memory.publish_status();
        auto *output = driver->dma.data() + driver->dmaSubframes * 160;
        for (unsigned i = 0; i < 80; ++i) {
            output[i * 2] = driver->memory.left[i];
            output[i * 2 + 1] = driver->memory.right[i];
        }
        if (++driver->dmaSubframes == JASDriver::getSubFrames()) {
            if (JASDriver::extMixCallback) {
                const aurora::allocation::ClientAllocationScope game({true, true});
                JASDriver::sMixFuncs[JASDriver::sMixMode](driver->dma.data(), JASDriver::getFrameSamples(), JASDriver::extMixCallback);
            }
            JASDriver::updateDacCallback();
#ifndef NDEBUG
            if (driver->pcmCapture)
                std::fwrite(driver->dma.data(), sizeof(s16), driver->dma.size(), driver->pcmCapture.get());
#endif
            driver->output.submit(driver->dma);
            driver->dmaSubframes = 0;
        }
        ++driver->generation;
        driver->frameReady.notify_all();
    }

    void start_dsp() {
        if (!driver || driver->worker.joinable())
            return;
        auto *state = driver.get();
        const aurora::allocation::HostAllocationScope host;
        state->worker = std::thread([state] {
            const aurora::allocation::HostAllocationScope host;
            try {
                while (!state->stopping) {
                    {
                        const aurora::os::GuestThreadExecutionScope execution;
                        if (state->stopping)
                            break;
                        const JKRHeap::CurrentHeapScope current(*JASDram);
                        const aurora::allocation::ClientAllocationScope game({true, true});
                        // Retain the original three-DMA-buffer lead. Audio device
                        // consumption drives subframes independently of rendering.
                        while (!state->paused && !state->stopping &&
                               state->output.queued_frames() < 3 * JASDriver::getFrameSamples())
                            render_subframe();
                    }
                    std::unique_lock lock(state->wakeMutex);
                    state->wake.wait_for(lock, std::chrono::milliseconds(3), [state] { return state->stopping.load(); });
                }
            } catch (...) {
                const aurora::os::GuestThreadExecutionScope execution;
                state->error = std::current_exception();
                state->frameReady.notify_all();
            }
        });
    }
    void check_dsp() {
        if (driver && driver->error)
            std::rethrow_exception(driver->error);
    }
    void wait_subframe() {
        if (!driver)
            return;
        check_dsp();
        if (!driver->worker.joinable() || driver->paused) {
            render_subframe();
            return;
        }
        const auto before = driver->generation.load();
        {
            const aurora::os::GuestThreadWaitScope waiting;
            std::unique_lock lock(driver->wakeMutex);
            driver->frameReady.wait_for(lock, std::chrono::milliseconds(10), [&] { return driver->generation.load() != before; });
        }
        check_dsp();
    }
    std::uint64_t nonzero_dsp_samples() {
        return driver ? driver->output.nonzero_samples() : 0;
    }
    void pause_dsp() {
        if (driver)
            driver->paused = true;
    }
    void shutdown_dsp() {
        if (!driver)
            return;
        driver->stopping = true;
        driver->wake.notify_all();
        if (driver->worker.joinable()) {
            const aurora::os::GuestThreadWaitScope waiting;
            driver->worker.join();
        }
        JASDvd::destroyThread();
        JASAramStream::sLoadThread = nullptr;
        JASDriver::sDspSyncCallback = {};
        JASDriver::sSubFrameCallback = {};
        JASDriver::sUpdateDacCallback = {};
        JASDriver::extMixCallback = nullptr;
        JASTrack::sTrackList.mCallbackRegistered = false;
        std::fprintf(stderr, "[original-audio] DSP submitted_frames=%llu nonzero_samples=%llu\n",
                     static_cast<unsigned long long>(driver->output.submitted_frames()),
                     static_cast<unsigned long long>(driver->output.nonzero_samples()));
        driver.reset();
    }
}  // namespace smgpc::audio
