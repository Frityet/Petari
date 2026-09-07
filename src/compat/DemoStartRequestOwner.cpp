#include "compat/DemoStartRequestOwner.hpp"

#include "Game/Demo/DemoStartRequestHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"

namespace smgpc::compat {

    DemoStartRequestOwner::DemoStartRequestOwner() {
        JkrHostAllocationScope host_allocations;
        _holder.reset(new DemoStartRequestHolder());
        claim_name_obj_runtime_ownership(_holder->mProxyObj, this);
    }

    DemoStartRequestOwner::~DemoStartRequestOwner() = default;

    DemoStartRequestHolder& DemoStartRequestOwner::get() noexcept {
        return *_holder;
    }

    const DemoStartRequestHolder& DemoStartRequestOwner::get() const noexcept {
        return *_holder;
    }

    void DemoStartRequestOwner::HolderDeleter::operator()(
        DemoStartRequestHolder* holder) const noexcept {
        if (holder == nullptr) {
            return;
        }
        for (s32 index = 0; index < holder->mNumInfos; ++index) {
            delete holder->mStartInfos[index];
        }
        delete holder->mProxyObj;
        delete holder;
    }

} // namespace smgpc::compat
