#pragma once
#include <cstdint>
#include <span>
namespace smgpc::audio {
    void initialize_dsp();
    void start_dsp();
    void check_dsp();
    void wait_subframe();
    void shutdown_dsp();
    void pause_dsp();
    void render_subframe();
    void upload_channel_batch(unsigned batch);
    std::uint64_t nonzero_dsp_samples();
}  // namespace smgpc::audio
