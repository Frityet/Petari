#pragma once

#include <cstdint>

namespace smgpc::game::title::runtime {

class Nerve;

class Spine {
public:
    Spine(void *executor, const Nerve *nerve);

    void update();
    void setNerve(const Nerve *nerve);
    [[nodiscard]] const Nerve *getCurrentNerve() const;
    [[nodiscard]] void *getExecutor() const;

    [[nodiscard]] std::int32_t getStep() const;

private:
    void changeNerve();

    void *mExecutor {};
    const Nerve *mCurrNerve {};
    const Nerve *mNextNerve {};
    std::int32_t mStep {};
};

}  // namespace smgpc::game::title::runtime
