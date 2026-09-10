#include <dolphin/os.h>
#include <dolphin/os/OSThread.h>

#include <aurora/allocation.hpp>
#include <aurora/guest_thread.hpp>

#include <bit>
#include <cstring>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstdlib>
#include <cstdio>

#include "thread.hpp"

namespace {

// Cooperative emulated-CPU ownership. This gate serializes callers of these
// SDK APIs; it does not suspend native workers or mask host signals. Interrupt
// state belongs to the calling context, while Reschedule is CPU-global as on
// the original SDK. Only the owning host thread ever unlocks the mutex.
std::mutex sCpuGate;
thread_local bool sOwnsCpu = false;
thread_local bool sInterruptsEnabled = true;
thread_local unsigned sGuestExecutionDepth = 0;
thread_local OSThread* sCurrentThread = nullptr;
void check_thread_control();
bool current_thread_cancelled();
u32 sReschedule = 0;

s32 scheduler_count() { return std::bit_cast<s32>(sReschedule); }

void acquire_cpu() {
  if (!sOwnsCpu) {
    sCpuGate.lock();
    sOwnsCpu = true;
    check_thread_control();
  }
}

void release_cpu() {
  sOwnsCpu = false;
  sCpuGate.unlock();
}

void release_cpu_if_enabled() {
  if (sInterruptsEnabled && scheduler_count() <= 0 && sGuestExecutionDepth == 0) {
    release_cpu();
  }
}

BOOL set_interrupts(bool enabled) {
  acquire_cpu();
  const BOOL previous = sInterruptsEnabled;
  sInterruptsEnabled = enabled;
  release_cpu_if_enabled();
  return previous;
}

} // namespace

BOOL OSDisableInterrupts() { return set_interrupts(false); }

BOOL OSEnableInterrupts() { return set_interrupts(true); }

BOOL OSRestoreInterrupts(BOOL level) { return set_interrupts(level != FALSE); }

s32 OSDisableScheduler() {
  const BOOL enabled = OSDisableInterrupts();
  const s32 count = scheduler_count();
  // Unsigned storage preserves the PowerPC addi wrap without signed C++ UB.
  ++sReschedule;
  OSRestoreInterrupts(enabled);
  return count;
}

s32 OSEnableScheduler() {
  const BOOL enabled = OSDisableInterrupts();
  const s32 count = scheduler_count();
  --sReschedule;
  OSRestoreInterrupts(enabled);
  return count;
}

void OSYieldThread() {
  const BOOL enabled = OSDisableInterrupts();
  if (scheduler_count() <= 0) {
    // SelectThread(TRUE) can switch even when the caller's saved interrupt
    // state is disabled. Retain that context's bit across the explicit yield.
    release_cpu();
    std::this_thread::yield();
    acquire_cpu();
  }
  OSRestoreInterrupts(enabled);
}


// Native thread identity and cooperative wait provider. Guest execution is
// serialized with host entry scopes and yields at explicit SDK boundaries.
namespace {
std::condition_variable sThreadWake;

[[noreturn]] void unsupported_thread_boundary(const char* reason) {
  std::fprintf(stderr, "Aurora OS thread boundary: %s\n", reason);
  std::abort();
}

void dequeue_thread(OSThreadQueue* queue, OSThread* thread) {
  auto* next = thread->link.next;
  auto* previous = thread->link.prev;
  if (next == nullptr) queue->tail = previous;
  else next->link.prev = previous;
  if (previous == nullptr) queue->head = next;
  else previous->link.next = next;
}

void enqueue_thread_by_priority(OSThreadQueue* queue, OSThread* thread) {
  auto* next = queue->head;
  while (next != nullptr && next->priority <= thread->priority) next = next->link.next;
  auto* previous = next == nullptr ? queue->tail : next->link.prev;
  thread->link.next = next;
  thread->link.prev = previous;
  if (next == nullptr) queue->tail = thread;
  else next->link.prev = thread;
  if (previous == nullptr) queue->head = thread;
  else previous->link.next = thread;
}

struct NativeThread {
  OSThread thread{};

  NativeThread() {
    // Native thread adoption uses __OSThreadInit's default-thread ownership
    // fields. There is no emulated PowerPC stack/context execution here.
    thread.state = OS_THREAD_STATE_RUNNING;
    thread.attr = OS_THREAD_ATTR_DETACH;
    thread.priority = thread.base = 16;
    thread.suspend = 0;
    thread.val = reinterpret_cast<void*>(~std::uintptr_t{0});
    thread.mutex = nullptr;
    OSInitThreadQueue(&thread.queueJoin);
    thread.queueMutex.head = thread.queueMutex.tail = nullptr;
    sCurrentThread = &thread;
  }

  ~NativeThread() {
    OSDisableInterrupts();
    if (scheduler_count() > 0) {
      unsupported_thread_boundary("thread exit while scheduler is disabled");
    }
    // Original OSExitThread / OSCancelThread release every owned mutex, even
    // recursively held ones. A native TLS destructor is the actual host exit.
    __OSUnlockAllMutex(&thread);
    thread.state = 0;
    sCurrentThread = nullptr;
    sInterruptsEnabled = true;
    release_cpu();
  }
};
thread_local NativeThread sNativeThread;

OSThread* set_effective_priority(OSThread* thread, OSPriority priority) {
  switch (thread->state) {
  case OS_THREAD_STATE_WAITING:
    dequeue_thread(thread->queue, thread);
    thread->priority = priority;
    enqueue_thread_by_priority(thread->queue, thread);
    if (thread->mutex != nullptr) return thread->mutex->thread;
    break;
  case OS_THREAD_STATE_READY:
  case OS_THREAD_STATE_RUNNING:
    thread->priority = priority;
    break;
  }
  return nullptr;
}

void update_priority(OSThread* thread) {
  do {
    if (thread->suspend > 0) break;
    const auto priority = __OSGetEffectivePriority(thread);
    if (thread->priority == priority) break;
    thread = set_effective_priority(thread, priority);
  } while (thread != nullptr);
}
} // namespace

OSThread* OSGetCurrentThread() { return sCurrentThread != nullptr ? sCurrentThread : &sNativeThread.thread; }

void OSInitThreadQueue(OSThreadQueue* queue) { queue->head = queue->tail = nullptr; }

void OSSleepThread(OSThreadQueue* queue) {
  const BOOL enabled = OSDisableInterrupts();
  if (scheduler_count() > 0) {
    unsupported_thread_boundary("blocking sleep while scheduler is disabled");
  }
  auto* current = OSGetCurrentThread();
  current->state = OS_THREAD_STATE_WAITING;
  current->queue = queue;
  enqueue_thread_by_priority(queue, current);

  // std::condition_variable atomically drops this same CPU mutex and waits;
  // wakeup edits the intrusive SDK queue under that mutex. No lost wakeup,
  // spinning, detached per-mutex owner, or second synchronization gate.
  std::unique_lock lock{sCpuGate, std::adopt_lock};
  sOwnsCpu = false;
  sThreadWake.wait(lock, [&] {
    return current_thread_cancelled() || (current->state != OS_THREAD_STATE_WAITING && current->suspend <= 0);
  });
  sOwnsCpu = true;
  lock.release();
  check_thread_control();
  current->state = OS_THREAD_STATE_RUNNING;
  current->queue = nullptr;
  OSRestoreInterrupts(enabled);
}

void OSWakeupThread(OSThreadQueue* queue) {
  const BOOL enabled = OSDisableInterrupts();
  while (queue->head != nullptr) {
    auto* thread = queue->head;
    dequeue_thread(queue, thread);
    thread->state = OS_THREAD_STATE_READY;
    // Actual native runnable threads wait in the host scheduler. No forged
    // original RunQueue pointer is published for an unimplemented scheduler.
    thread->queue = nullptr;
  }
  sThreadWake.notify_all();
  OSRestoreInterrupts(enabled);
}

OSPriority __OSGetEffectivePriority(OSThread* thread) {
  OSPriority priority = thread->base;
  for (OSMutex* mutex = thread->queueMutex.head; mutex; mutex = mutex->link.next) {
    OSThread* blocked = mutex->queue.head;
    if (blocked != nullptr && blocked->priority < priority) priority = blocked->priority;
  }
  return priority;
}

void __OSPromoteThread(OSThread* thread, OSPriority priority) {
  do {
    if (thread->suspend > 0 || thread->priority <= priority) break;
    thread = set_effective_priority(thread, priority);
  } while (thread != nullptr);
}

BOOL OSSetThreadPriority(OSThread* thread, OSPriority priority) {
  if (priority < OS_PRIORITY_MIN || priority > OS_PRIORITY_MAX) return FALSE;
  const BOOL enabled = OSDisableInterrupts();
  if (thread->base != priority) {
    thread->base = priority;
    update_priority(thread);
  }
  OSRestoreInterrupts(enabled);
  return TRUE;
}

s32 OSGetThreadPriority(OSThread* thread) {
  // The retail aligned word load is atomic; participating native callers also
  // need the shared gate to synchronize with priority changes on other hosts.
  const BOOL enabled = OSDisableInterrupts();
  const auto priority = thread->base;
  OSRestoreInterrupts(enabled);
  return priority;
}


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
    const aurora::allocation::ClientAllocationScope client_callback;
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
