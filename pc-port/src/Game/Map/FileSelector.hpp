#pragma once

#include <cstdint>
#include <memory>

#include <revolution.h>

#include "Game/Screen/TitleBackground.hpp"
#include "Game/System/NerveExecutor.hpp"

class HitSensor;
class TitleSequenceProduct;

class FileSelector : public NerveExecutor {
public:
    FileSelector();
    ~FileSelector();

    void update();
    void draw(smgpc::render::IRendererEngine& renderer);
    bool receiveOtherMsg(u32 msg, HitSensor* pSender = nullptr, HitSensor* pReceiver = nullptr);

    void createTitle();
    void createSky();
    void exeWaitBind();
    void exeTitle();
    void exeTitleEnd();
    void exeRFLError();
    void exeRFLWait();

    [[nodiscard]] std::uint64_t getSkyStep() const;
    [[nodiscard]] bool isTitleActive() const;
    [[nodiscard]] bool isTitleStarted() const;
    [[nodiscard]] bool isTitleEnded() const;

private:
    std::unique_ptr< TitleSequenceProduct > mTitleSeq;
    smgpc::game::TitleBackground mTitleBackground;
    std::uint64_t mSkyStep = 0U;
    bool mSkyAppeared = false;
    bool mTitleStarted = false;
    bool mTitleEnded = false;
};
