#include "proof.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
namespace ProofNerves {
    INIT_NERVE(Plain);
    INIT_NERVE(PlainEnd);
    INIT_NERVE(Declared);
    INIT_NERVE(Execute);
    INIT_NERVE(OnEnd);
    INIT_NERVE(Null);
    INIT_NERVE(Unavailable);
    INIT_NERVE(Unused);
}
namespace { NERVE_DECL_EXE(Anonymous, ProofActor, Wait); }
void ProofNerves::Plain::execute(Spine* spine) const { static_cast<ProofActor*>(spine->mExecutor)->execute(); }
void ProofNerves::PlainEnd::execute(Spine* spine) const { static_cast<ProofActor*>(spine->mExecutor)->execute(); }
void ProofNerves::PlainEnd::executeOnEnd(Spine* spine) const { static_cast<ProofActor*>(spine->mExecutor)->end(); }
int main() {
    ProofActor actor;
    constexpr Anonymous anonymous;
    const Nerve* nerve = proofOtherTU();
    assert(nerve == &ProofNerves::OnEnd::sInstance);
    Spine spine(&actor, nerve);
    nerve->execute(&spine);
    nerve->executeOnEnd(&spine);
    assert(actor.calls == 1 && actor.ends == 1);
    const Nerve* simple[] = {&ProofNerves::Plain::sInstance, &ProofNerves::PlainEnd::sInstance,
                            &ProofNerves::Declared::sInstance, &ProofNerves::Execute::sInstance};
    for (const Nerve* state : simple) { state->execute(&spine); state->executeOnEnd(&spine); }
    assert(actor.calls == 5 && actor.ends == 2);
    assert(ProofNerves::Null::get() == &ProofNerves::Null::sInstance);
    const Nerve* empty = ProofNerves::Null::get();
    empty->execute(&spine);
    empty->executeOnEnd(&spine);
    assert(actor.calls == 5 && actor.ends == 2);
    bool threw = false;
    try { static_cast<const Nerve*>(&ProofNerves::Unavailable::sInstance)->execute(&spine); }
    catch (const std::logic_error& error) { threw = std::strcmp(error.what(), "proof unavailable") == 0; }
    assert(threw);
    std::puts("[ok] constant Nerve initialization: shared singleton identity, all seven constructors, virtual execute/end, and unused unresolved executor discarded");
}
