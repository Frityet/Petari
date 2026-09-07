#pragma once

class EffectSystem;
class NameObjAdaptor;

class ParticleCalcExecutor {
public:
    ParticleCalcExecutor(const EffectSystem*, bool);

    void movementNormal();
    void movementIgnorePause3D();
    void movementIgnorePause2D();
    void movementCheckUpdate();
    void requestMovementOnPauseIgnore();
    void initMovementAdaptor();

    /* 0x00 */ const EffectSystem* mHost;
    /* 0x04 */ NameObjAdaptor* _4;
    /* 0x08 */ NameObjAdaptor* _8;
    /* 0x0C */ NameObjAdaptor* _C;
    /* 0x10 */ NameObjAdaptor* _10;
    /* 0x14 */ bool _14;
    /* 0x15 */ bool _15;
};
