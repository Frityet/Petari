#pragma once

namespace smgpc::game::title::runtime {

class Spine;

class Nerve {
public:
    virtual ~Nerve() = default;
    virtual void execute(Spine *spine) const = 0;
    virtual void execute_on_end(Spine *spine) const;
};

}  // namespace smgpc::game::title::runtime
