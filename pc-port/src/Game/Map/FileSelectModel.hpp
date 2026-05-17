#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class FileSelectModel : public LiveActor {
public:
    FileSelectModel(const char*, MtxPtr, const char*);
    ~FileSelectModel() override;

    void calcAnim() override;
    void calcAndSetBaseMtx() override;

    void open();
    void blinkOnce();
    void close();
    void blink();
    bool isOpen() const;
    void emitOpen();
    void emitVanish();
    void emitCopy();
    void emitCompleteEffect();
    void deleteCompleteEffect();
    void exeOpen();
    void exeBlinkOnce();
    void exeClose();
    void exeBlink();

    MtxPtr _8C;
};
