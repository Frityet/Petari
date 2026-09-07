#pragma once
#include "Game/LiveActor/Nerve.hpp"

struct ProofActor {
    int calls = 0;
    int ends = 0;
    void execute() { ++calls; }
    void exeWait() { ++calls; }
    void end() { ++ends; }
    void exeMissing();
};
namespace ProofNerves {
    NERVE(Plain);
    NERVE_EXECEND(PlainEnd);
    NERVE_DECL(Declared, ProofActor, execute);
    NERVE_DECL_EXE(Execute, ProofActor, Wait);
    NERVE_DECL_ONEND(OnEnd, ProofActor, execute, end);
    NERVE_DECL_NULL(Null);
    NERVE_DECL_UNAVAILABLE(Unavailable, "proof unavailable");
    // This body has no definition: dead stripping must discard its unused singleton.
    NERVE_DECL_EXE(Unused, ProofActor, Missing);
    constexpr Plain checkPlain;
    constexpr PlainEnd checkPlainEnd;
    constexpr Declared checkDeclared;
    constexpr Execute checkExecute;
    constexpr OnEnd checkOnEnd;
    constexpr Null checkNull;
    constexpr Unavailable checkUnavailable;
}
const Nerve* proofOtherTU();
