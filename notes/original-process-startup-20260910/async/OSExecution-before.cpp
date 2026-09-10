#include <dolphin/os.h>
#include <dolphin/os/OSThread.h>

#include <bit>
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
u32 sReschedule = 0;

s32 scheduler_count() { return std::bit_cast<s32>(sReschedule); }

void acquire_cpu() {
  if (!sOwnsCpu) {
    sCpuGate.lock();
    sOwnsCpu = true;
  }
}

void release_cpu() {
  sOwnsCpu = false;
  sCpuGate.unlock();
}

void release_cpu_if_enabled() {
  if (sInterruptsEnabled && scheduler_count() <= 0) {
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


// Native thread identity / wait provider. No OSCreateThread or native
// preemptive scheduler is supplied by this boundary.
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

OSThread* OSGetCurrentThread() { return &sNativeThread.thread; }

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
  sThreadWake.wait(lock, [&] { return current->state != OS_THREAD_STATE_WAITING; });
  sOwnsCpu = true;
  lock.release();
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
