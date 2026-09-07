#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SystemUtil.hpp"

J2DOrthoGraphSimple::J2DOrthoGraphSimple()
    : J2DOrthoGraph(0.0f, 0.0f, MR::getFrameBufferWidth(), MR::getScreenHeight(), -30000.0f, 30000.0f) {
    TBox2f bounds(0.0f, 0.0f, MR::getScreenWidth(), MR::getScreenHeight());
    setOrtho(TBox2f(0.0f, 0.0f, 0.0f + bounds.getWidth(), 0.0f + bounds.getHeight()), -30000.0f, 30000.0f);
}

void J2DOrthoGraphSimple::setPort() {
    J2DOrthoGraph::setPort();
}

namespace MR {
    void loadProjectionMtxFor2D() {
        J2DOrthoGraphSimple orthoGraph;
        orthoGraph.setPort();
    }
}
