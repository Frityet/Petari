
namespace {
struct ThreadExit {
  void* value;
};

struct ManagedThread {
  OSThread* thread;
  void* (*function)(void*);
  void* argument;
  aurora::allocation::RoutingState routing;
  std::thread native;
  bool cancel = false;
  bool finished = false;
};

std::map<OSThread*, std::unique_ptr<ManagedThread>> sManagedThreads;
thread_local ManagedThread* sManagedThread = nullptr;

void wait_for_control_change() {
  if (scheduler_count() > 0) {
    unsupported_thread_boundary("blocking thread control while scheduler is disabled");
  }
  std::unique_lock lock{sCpuGate, std::adopt_lock};
  sOwnsCpu = false;
  sThreadWake.wait(lock);
  sOwnsCpu = true;
  lock.release();
}

bool current_thread_cancelled() {
  return sManagedThread != nullptr && sManagedThread->cancel;
}

void check_thread_control() {
  if (current_thread_cancelled()) throw ThreadExit{reinterpret_cast<void*>(~std::uintptr_t{0})};
  if (sCurrentThread == nullptr) return;
  while (sCurrentThread->suspend > 0) {
    if (sCurrentThread->state == OS_THREAD_STATE_RUNNING) sCurrentThread->state = OS_THREAD_STATE_READY;
    wait_for_control_change();
    if (current_thread_cancelled()) throw ThreadExit{reinterpret_cast<void*>(~std::uintptr_t{0})};
  }
  if (sCurrentThread->state == OS_THREAD_STATE_READY) sCurrentThread->state = OS_THREAD_STATE_RUNNING;
}

void run_managed_thread(ManagedThread* record) {
  sManagedThread = record;
  sCurrentThread = record->thread;
  void* value = reinterpret_cast<void*>(~std::uintptr_t{0});
  // Acquire without the checkpoint: the initial suspended wait belongs inside
  // this exit handler, including cancellation before the first resume.
  sCpuGate.lock();
  sOwnsCpu = true;
  ++sGuestExecutionDepth;
  try {
    check_thread_control();
    aurora::allocation::routing_state = record->routing;
    value = record->function(record->argument);
  } catch (const ThreadExit& exit) {
    value = exit.value;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "Aurora OS guest thread failed: %s\n", error.what());
    std::abort();
  } catch (...) {
    unsupported_thread_boundary("unhandled exception from guest thread entry point");
  }
  aurora::allocation::routing_state = {};
  // All original locks and join queues are owned by this same CPU gate.
  // Disable control checkpoints while publishing the completed thread state.
  sManagedThread = nullptr;
  __OSUnlockAllMutex(record->thread);
  record->thread->val = value;
  record->thread->state = (record->thread->attr & OS_THREAD_ATTR_DETACH) ? 0 : OS_THREAD_STATE_MORIBUND;
  record->thread->queue = nullptr;
  OSWakeupThread(&record->thread->queueJoin);
  record->finished = true;
  sThreadWake.notify_all();
  --sGuestExecutionDepth;
  sInterruptsEnabled = true;
  sCurrentThread = nullptr;
  release_cpu();
}

// A detached SDK thread is still joined internally before its caller may
// release OSThread/stack storage. Detach changes SDK joinability, not native
// ownership. This avoids destroying a live wrapper after cooperative cancel.
void reap_managed_thread(OSThread* thread) {
  auto found = sManagedThreads.find(thread);
  if (found == sManagedThreads.end() || !found->second->finished) return;
  aurora::allocation::HostAllocationScope host;
  auto record = std::move(found->second);
  sManagedThreads.erase(found);
  release_cpu();
  record->native.join();
  acquire_cpu();
}
} // namespace

namespace aurora::os {
GuestThreadExecutionScope::GuestThreadExecutionScope() {
  acquire_cpu();
  ++sGuestExecutionDepth;
}
GuestThreadExecutionScope::~GuestThreadExecutionScope() {
  --sGuestExecutionDepth;
  release_cpu_if_enabled();
}
} // namespace aurora::os

BOOL OSCreateThread(OSThread* thread, void* (*function)(void*), void* argument, void* stack,
                    u32 stackSize, OSPriority priority, u16 attributes) {
  if (thread == nullptr || function == nullptr || stack == nullptr || stackSize < sizeof(u32) ||
      priority < OS_PRIORITY_MIN || priority > OS_PRIORITY_MAX) return FALSE;
  const auto routing = aurora::allocation::routing_state;
  const BOOL enabled = OSDisableInterrupts();
  aurora::allocation::HostAllocationScope host;
  auto previous = sManagedThreads.find(thread);
  if (previous != sManagedThreads.end()) {
    if (!previous->second->finished) {
      OSRestoreInterrupts(enabled);
      return FALSE;
    }
    reap_managed_thread(thread);
  }
  *thread = {};
  thread->state = OS_THREAD_STATE_READY;
  thread->attr = attributes & OS_THREAD_ATTR_DETACH;
  thread->suspend = 1;
  thread->priority = thread->base = priority;
  thread->val = reinterpret_cast<void*>(~std::uintptr_t{0});
  thread->stackBase = static_cast<u8*>(stack);
  thread->stackEnd = thread->stackBase - stackSize;
  const u32 magic = OS_THREAD_STACK_MAGIC;
  std::memcpy(thread->stackEnd, &magic, sizeof(magic));
  try {
    auto record = std::make_unique<ManagedThread>();
    record->thread = thread;
    record->function = function;
    record->argument = argument;
    record->routing = routing;
    auto* pointer = record.get();
    sManagedThreads.emplace(thread, std::move(record));
    pointer->native = std::thread(run_managed_thread, pointer);
  } catch (...) {
    sManagedThreads.erase(thread);
    thread->state = 0;
    OSRestoreInterrupts(enabled);
    return FALSE;
  }
  OSRestoreInterrupts(enabled);
  return TRUE;
}

s32 OSResumeThread(OSThread* thread) {
  const BOOL enabled = OSDisableInterrupts();
  const s32 previous = thread->suspend;
  thread->suspend = std::bit_cast<s32>(std::bit_cast<u32>(previous) - 1U);
  if (thread->suspend < 0) thread->suspend = 0;
  else if (thread->suspend == 0) {
    if (thread->state == OS_THREAD_STATE_READY) thread->priority = __OSGetEffectivePriority(thread);
    else if (thread->state == OS_THREAD_STATE_WAITING) {
      dequeue_thread(thread->queue, thread);
      thread->priority = __OSGetEffectivePriority(thread);
      enqueue_thread_by_priority(thread->queue, thread);
      if (thread->mutex != nullptr) update_priority(thread->mutex->thread);
    }
    sThreadWake.notify_all();
  }
  OSRestoreInterrupts(enabled);
  return previous;
}

s32 OSSuspendThread(OSThread* thread) {
  const BOOL enabled = OSDisableInterrupts();
  const s32 previous = thread->suspend;
  thread->suspend = std::bit_cast<s32>(std::bit_cast<u32>(previous) + 1U);
  if (previous == 0) {
    if (thread->state == OS_THREAD_STATE_RUNNING) thread->state = OS_THREAD_STATE_READY;
    else if (thread->state == OS_THREAD_STATE_WAITING) {
      dequeue_thread(thread->queue, thread);
      thread->priority = OS_PRIORITY_MAX + 1;
      // Suspended waiters follow every normal-priority waiter.
      enqueue_thread_by_priority(thread->queue, thread);
      if (thread->mutex != nullptr) update_priority(thread->mutex->thread);
    }
    if (thread == OSGetCurrentThread()) check_thread_control();
  }
  OSRestoreInterrupts(enabled);
  return previous;
}

BOOL OSIsThreadSuspended(OSThread* thread) {
  const BOOL enabled = OSDisableInterrupts();
  const BOOL result = thread->suspend > 0;
  OSRestoreInterrupts(enabled);
  return result;
}

BOOL OSIsThreadTerminated(OSThread* thread) {
  const BOOL enabled = OSDisableInterrupts();
  const BOOL result = thread->state == 0 || thread->state == OS_THREAD_STATE_MORIBUND;
  if (result && (thread->attr & OS_THREAD_ATTR_DETACH)) reap_managed_thread(thread);
  OSRestoreInterrupts(enabled);
  return result;
}

void OSExitThread(void* value) {
  if (sManagedThread == nullptr) unsupported_thread_boundary("OSExitThread requires an OS-created native thread");
  throw ThreadExit{value};
}

void OSCancelThread(OSThread* thread) {
  const BOOL enabled = OSDisableInterrupts();
  auto found = sManagedThreads.find(thread);
  if (found == sManagedThreads.end()) {
    if (thread->state != 0 && thread->state != OS_THREAD_STATE_MORIBUND) {
      unsupported_thread_boundary("cannot cancel an adopted host thread");
    }
    OSRestoreInterrupts(enabled);
    return;
  }
  auto* record = found->second.get();
  record->cancel = true;
  if (thread == OSGetCurrentThread()) throw ThreadExit{reinterpret_cast<void*>(~std::uintptr_t{0})};
  if (thread->state == OS_THREAD_STATE_WAITING) {
    dequeue_thread(thread->queue, thread);
    thread->queue = nullptr;
    thread->state = OS_THREAD_STATE_READY;
    if (thread->mutex != nullptr) update_priority(thread->mutex->thread);
  }
  sThreadWake.notify_all();
  while (!record->finished) wait_for_control_change();
  reap_managed_thread(thread);
  OSRestoreInterrupts(enabled);
}

BOOL OSJoinThread(OSThread* thread, void** value) {
  const BOOL enabled = OSDisableInterrupts();
  if (!(thread->attr & OS_THREAD_ATTR_DETACH) && thread->state != OS_THREAD_STATE_MORIBUND &&
      thread->state != 0 && thread->queueJoin.head == nullptr) {
    OSSleepThread(&thread->queueJoin);
  }
  if (thread->state == OS_THREAD_STATE_MORIBUND) {
    if (value != nullptr) *value = thread->val;
    thread->state = 0;
    reap_managed_thread(thread);
    OSRestoreInterrupts(enabled);
    return TRUE;
  }
  OSRestoreInterrupts(enabled);
  return FALSE;
}

void OSDetachThread(OSThread* thread) {
  const BOOL enabled = OSDisableInterrupts();
  thread->attr |= OS_THREAD_ATTR_DETACH;
  if (thread->state == OS_THREAD_STATE_MORIBUND) thread->state = 0;
  OSWakeupThread(&thread->queueJoin);
  if (thread->state == 0) reap_managed_thread(thread);
  OSRestoreInterrupts(enabled);
}

void OSSetThreadSpecific(s32 index, void* value) {
  if (index < 0 || index >= OS_THREAD_SPECIFIC_MAX) unsupported_thread_boundary("thread-specific index out of range");
  OSGetCurrentThread()->specific[index] = value;
}

void* OSGetThreadSpecific(s32 index) {
  if (index < 0 || index >= OS_THREAD_SPECIFIC_MAX) unsupported_thread_boundary("thread-specific index out of range");
  return OSGetCurrentThread()->specific[index];
}
