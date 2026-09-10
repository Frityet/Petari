#pragma once
#include <aurora/audio.hpp>
#include <JSystem/JAudio2/JASAramStream.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace aurora::audio {
class JasStreamPcmBackend final {
public:
    using StreamLoader = std::function<std::vector<std::uint8_t>(s32)>;
    JasStreamPcmBackend(PcmAudioMixer&, StreamLoader);
    ~JasStreamPcmBackend();
    JasStreamPcmBackend(const JasStreamPcmBackend&) = delete;
    JasStreamPcmBackend& operator=(const JasStreamPcmBackend&) = delete;
    class Binding final {
    public:
        explicit Binding(JasStreamPcmBackend&);
        ~Binding();
        Binding(const Binding&) = delete;
        Binding& operator=(const Binding&) = delete;
    private:
        JasStreamPcmBackend* previous_;
    };
    static JasStreamPcmBackend& active();
    void initialize(JASAramStream&);
    bool prepare(JASAramStream&, s32, int);
    bool cancel(JASAramStream&);
    void update();
    void shutdown();
    [[nodiscard]] VoiceToken voice(const JASAramStream&) const;
    [[nodiscard]] std::size_t stream_count() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
