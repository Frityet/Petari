#include "Game/Player/DrawAdaptor.hpp"
#include "Game/Util/ObjUtil.hpp"

DrawAdaptor::DrawAdaptor(const MR::FunctorBase& rFunctor, int drawType) : NameObj("ドロー2D") {
    mFunctor = rFunctor.clone(nullptr);
    MR::connectToScene(this, -1, -1, -1, drawType);
}

DrawAdaptor::~DrawAdaptor() {}

void DrawAdaptor::draw() const {
    (*mFunctor)();
}
