#include "RuntimeContext.hpp"

namespace smgpc::game::compat {
namespace {

RuntimeContext rt_ctx {};

}  // namespace

void set_runtime_context(RuntimeContext context) {
    rt_ctx = context;
}

const RuntimeContext &runtime_context() {
    return rt_ctx;
}

}  // namespace smgpc::game::compat
