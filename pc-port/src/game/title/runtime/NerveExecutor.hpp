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

    void initNerve(const Nerve *nerve);
    void updateNerve();
    void setNerve(const Nerve *nerve);
    [[nodiscard]] bool isNerve(const Nerve *nerve) const;
    [[nodiscard]] std::int32_t getNerveStep() const;

protected:
    [[nodiscard]] Spine *getSpine() const;

private:
    std::unique_ptr<Spine> mSpine {};
};

}  // namespace smgpc::game::title::runtime
