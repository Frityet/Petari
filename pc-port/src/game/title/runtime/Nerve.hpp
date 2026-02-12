#pragma once

namespace smgpc::game::title::runtime {

class Spine;

class Nerve {
public:
    virtual ~Nerve() = default;
    virtual void execute(Spine *spine) const = 0;
    virtual void executeOnEnd(Spine *spine) const;
};

}  // namespace smgpc::game::title::runtime
