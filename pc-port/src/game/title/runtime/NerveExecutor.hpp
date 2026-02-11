#pragma once

#include <cstdint>
#include <memory>

namespace smgpc::game::title::runtime {

class Nerve;
class Spine;

class NerveExecutor {
public:
    NerveExecutor();
    virtual ~NerveExecutor();

    void init_nerve(const Nerve *nerve);
    void update_nerve();
    void set_nerve(const Nerve *nerve);
    [[nodiscard]] bool is_nerve(const Nerve *nerve) const;
    [[nodiscard]] std::int32_t nerve_step() const;

protected:
    [[nodiscard]] Spine *spine() const;

private:
    std::unique_ptr<Spine> _spine {};
};

}  // namespace smgpc::game::title::runtime
