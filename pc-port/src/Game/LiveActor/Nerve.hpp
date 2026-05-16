#pragma once

#include "Game/LiveActor/Spine.hpp"

class Nerve {
public:
    virtual void execute(Spine* pSpine) const = 0;
    virtual void executeOnEnd(Spine* pSpine) const;
};

#define NEW_NERVE(name, parent_class, executor_name)                                                                                                 \
    class name : public Nerve {                                                                                                                      \
    public:                                                                                                                                          \
        virtual void execute(Spine* pSpine) const {                                                                                                  \
            parent_class* actor = reinterpret_cast< parent_class* >(pSpine->mExecutor);                                                              \
            actor->exe##executor_name();                                                                                                             \
        };                                                                                                                                           \
        static name sInstance;                                                                                                                       \
    };                                                                                                                                               \
    name name::sInstance;

