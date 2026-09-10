#pragma once

class FunctionAsyncExecutor;

namespace smgpc::compat {

// Retire the original executor only after the process has stopped submitting
// work. Workers finish cooperative cancellation before any queue/task heap
// storage is released. The caller clears its original owning pointer.
void destroy_function_async_executor(FunctionAsyncExecutor* executor);

}
