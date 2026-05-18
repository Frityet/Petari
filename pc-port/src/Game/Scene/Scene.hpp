#pragma once

#include "Game/System/NerveExecutor.hpp"

class SceneNameObjListExecutor;
class SceneObjHolder;

class Scene : public NerveExecutor {
public:
    explicit Scene(const char* pName);
    ~Scene() override;

    virtual void init();
    virtual void start();
    virtual void update();
    virtual void draw() const;
    virtual void calcAnim();

    void initNameObjListExecutor();
    void initSceneObjHolder();

    /* 0x08 */ SceneNameObjListExecutor* mListExecutor;
    /* 0x0C */ u32 _C;
    /* 0x10 */ SceneObjHolder* mSceneObjHolder;
};
