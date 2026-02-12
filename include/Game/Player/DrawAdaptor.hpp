#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"

class DrawAdaptor : public NameObj {
public:
    DrawAdaptor(const MR::FunctorBase&, int);

    virtual ~DrawAdaptor();
    virtual void draw() const;

    MR::FunctorBase* mFunctor;  // 0xC
};
