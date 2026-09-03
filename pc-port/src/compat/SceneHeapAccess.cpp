#include "Game/Util/MemoryUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include <stdexcept>

namespace MR {
    JKRSolidHeap* getSceneHeapGDDR3() {
        const auto* resources = smgpc::compat::ResourceHolderService::active();
        if (!resources) {
            throw std::logic_error("Scene heap access requires an active scene resource owner");
        }
        // The native scene cohort is an actual JKRSolidHeap. Its owner retains
        // it with the resources, models and display lists allocated within it.
        return static_cast<JKRSolidHeap*>(&resources->allocation_domain()->heap());
    }
}
