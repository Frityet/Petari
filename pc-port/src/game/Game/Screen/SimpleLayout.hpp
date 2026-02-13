#pragma once

#include <memory>

#include "Game/Screen/LayoutActor.hpp"

namespace smgpc::game::layout {
class LayoutRuntimeActor;
}

class SimpleLayout : public LayoutActor {
public:
    SimpleLayout(const char *pName, const char *pArchiveName, int a3, int a4);

    void initWithoutIter();

private:
    [[nodiscard]] static std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> loadRuntimeActor(const char *pArchiveName);
};
