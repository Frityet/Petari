#pragma once

#include <cstdint>

namespace smgpc::game::title::runtime {

class Nerve;

class Spine {
public:
    Spine(void *executor, const Nerve *nerve);

    void update();
    void set_nerve(const Nerve *nerve);
    [[nodiscard]] const Nerve *current_nerve() const;
    [[nodiscard]] void *executor() const;

    [[nodiscard]] std::int32_t step() const;

private:
    void change_nerve();

    void *_executor {};
    const Nerve *_current_nerve {};
    const Nerve *_next_nerve {};
    std::int32_t _step {};
};

}  // namespace smgpc::game::title::runtime
