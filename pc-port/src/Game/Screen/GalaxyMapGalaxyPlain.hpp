#pragma once

#include "Game/Screen/LayoutActor.hpp"

class GalaxyMapGalaxyPlain : public LayoutActor {
public:
    explicit GalaxyMapGalaxyPlain(const LayoutActor* pHost);

    void init(const JMapInfoIter& rIter) override;
    void control() override;
    void show(const char* pMessage, const char* pPaneName);
    void show(const wchar_t* pMessage, const char* pPaneName);
    [[nodiscard]] const char* getFollowPaneName() const;

private:
    /* 0x20 */ const LayoutActor* mHost;
    /* 0x24 */ const char* mPaneName;
};
