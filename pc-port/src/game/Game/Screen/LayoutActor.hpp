#pragma once

#include <memory>

#include "layout/LayoutDrawList.hpp"

namespace smgpc::game::layout {
struct LayoutArchiveData;
class LayoutRuntimeActor;
}

class LayoutActor {
public:
    LayoutActor(const char *pName, std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> runtime_actor);
    virtual ~LayoutActor() = default;

    virtual void movement();
    virtual void draw() const;
    virtual void calcAnim();
    virtual void appear();
    virtual void kill();

    virtual void control() {
    }

    void startAnim(const char *pAnimName, unsigned int layer);
    [[nodiscard]] bool isAnimStopped(unsigned int layer) const;
    void setAnimFrameAndStop(float frame, unsigned int layer);
    void emitEffect(const char *pEffectName);
    void deleteEffectAll();
    [[nodiscard]] bool isDead() const;

    void appendDrawCommands(smgpc::render::layout::LayoutDrawList *pDrawList) const;

    [[nodiscard]] const smgpc::game::layout::LayoutArchiveData *getResource() const;
    [[nodiscard]] const char *getName() const;

protected:
    /* 0x08 */ const char *mName;
    /* 0x10 */ std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> mRuntimeActor;
};
