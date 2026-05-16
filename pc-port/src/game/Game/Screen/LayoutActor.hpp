#pragma once

#include <memory>
#include <string>

#include "compat/Types.hpp"
#include "layout/LayoutDrawList.hpp"

class Nerve;
class Spine;

namespace nw4r::lyt {
class TexMap;
}

namespace smgpc::game::layout {
struct LayoutArchiveData;
class LayoutRuntimeActor;
}

class LayoutActor {
public:
    LayoutActor(const char *pName, bool connectToScene);
    LayoutActor(const char *pName, std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> runtime_actor);
    virtual ~LayoutActor();

    virtual void movement();
    virtual void draw() const;
    virtual void calcAnim();
    virtual void appear();
    virtual void kill();

    virtual void control() {
    }

    void setNerve(const Nerve *pNerve) const;
    [[nodiscard]] bool isNerve(const Nerve *pNerve) const;
    [[nodiscard]] s32 getNerveStep() const;
    void initNerve(const Nerve *pNerve);
    void updateSpine();

    [[nodiscard]] TVec2f getTrans() const;
    void setTrans(const TVec2f &rTrans);
    void initLayoutManager(const char *pArchiveName, u32 groupCount);
    void setPaneVisible(const char *pPaneName, bool visible);
    void setPaneVisibleRecursive(const char *pPaneName, bool visible);
    void setTextBoxTextRecursive(const char *pPaneName, std::u16string text);
    void clearTextBoxTextRecursive(const char *pPaneName);
    void setTextBoxVerticalPositionRecursive(const char *pPaneName, u8 position);
    [[nodiscard]] bool getPaneTrans(const char *pPaneName, TVec2f *pOut) const;
    [[nodiscard]] bool getPaneBounds(const char *pPaneName, f32 *pX0, f32 *pY0, f32 *pX1, f32 *pY1) const;
    void setPaneFollowPos(const char *pPaneName, const TVec2f *pFollowPos) const;
    void replacePaneTexture(const char *pPaneName, const nw4r::lyt::TexMap *pTexMap, u8 slot);
    [[nodiscard]] nw4r::lyt::TexMap *getPaneTexture(const char *pPaneName, u8 slot) const;
    [[nodiscard]] bool isExistPaneCtrl(const char *pPaneName) const;
    void startPaneAnim(const char *pPaneName, const char *pAnimName, u32 slot);
    [[nodiscard]] bool isPaneAnimStopped(const char *pPaneName, u32 slot) const;
    void setPaneAnimFrame(const char *pPaneName, f32 frame, u32 slot);
    [[nodiscard]] f32 getPaneAnimFrame(const char *pPaneName, u32 slot) const;
    void setPaneAnimRate(const char *pPaneName, f32 rate, u32 slot);

    void startAnim(const char *pAnimName, unsigned int layer);
    [[nodiscard]] bool isAnimStopped(unsigned int layer) const;
    void setAnimFrameAndStop(float frame, unsigned int layer);
    void setAnimFrame(float frame, unsigned int layer);
    void setAnimRate(float rate, unsigned int layer);
    [[nodiscard]] float getAnimFrame(unsigned int layer) const;
    [[nodiscard]] float getAnimRate(unsigned int layer) const;
    [[nodiscard]] float getAnimFrameMax(unsigned int layer) const;
    [[nodiscard]] float getAnimFrameMax(const char *pAnimName) const;
    void emitEffect(const char *pEffectName);
    void deleteEffectAll();
    [[nodiscard]] bool isDead() const;

    void appendDrawCommands(smgpc::render::layout::LayoutDrawList *pDrawList) const;

    [[nodiscard]] const smgpc::game::layout::LayoutArchiveData *getResource() const;
    [[nodiscard]] const char *getName() const;

protected:
    /* 0x08 */ const char *mName;
    /* 0x10 */ std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> mRuntimeActor;
    /* 0x20 */ mutable Spine *mSpine;
    /* 0x28 */ TVec2f mTrans;
};
