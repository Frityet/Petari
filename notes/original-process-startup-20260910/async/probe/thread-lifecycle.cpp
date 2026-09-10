#include <aurora/allocation.hpp>
#include <aurora/guest_thread.hpp>
#include <dolphin/os.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {
unsigned checks = 0;
void require(bool value, const char* message) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::fflush(stderr);
    std::abort();
  }
  ++checks;
}
void* token(std::uintptr_t value) { return reinterpret_cast<void*>(value); }

struct Handoff {
  OSThread thread{};
  alignas(32) std::array<u8, 0x10000> stack{};
  OSMessageQueue commands{};
  OSMessageQueue replies{};
  OSMessage commandStorage[2]{};
  OSMessage replyStorage[2]{};
  bool entered = false;
  bool completed = false;
};
void* handoff(void* argument) {
  auto& data = *static_cast<Handoff*>(argument);
  data.entered = true;
  require(OSGetCurrentThread() == &data.thread, "worker has the actual supplied OSThread identity");
  require(aurora::allocation::routing_state.guest, "worker inherits guest callback allocation routing");
  require(OSSendMessage(&data.replies, token(1), OS_MESSAGE_BLOCK), "worker publishes entry");
  OSMessage value = nullptr;
  require(OSReceiveMessage(&data.commands, &value, OS_MESSAGE_BLOCK), "worker receives queued message");
  require(value == token(0x1234), "queued payload survives suspended startup");
  require(OSSendMessage(&data.replies, value, OS_MESSAGE_BLOCK), "worker acknowledges exact payload");
  require(OSReceiveMessage(&data.commands, &value, OS_MESSAGE_BLOCK), "worker receives completion request");
  require(value == token(0x5678), "completion payload survives nested suspension");
  data.completed = true;
  return token(0x9876);
}

struct Cancellation {
  OSThread thread{};
  alignas(32) std::array<u8, 0x10000> stack{};
  OSMutex mutex{};
  OSMessageQueue ready{};
  OSMessageQueue blocked{};
  OSMessage readyStorage[1]{};
  OSMessage blockedStorage[1]{};
  bool escapedWait = false;
};
void* cancellation(void* argument) {
  auto& data = *static_cast<Cancellation*>(argument);
  OSLockMutex(&data.mutex);
  OSLockMutex(&data.mutex);
  require(OSSendMessage(&data.ready, token(1), OS_MESSAGE_BLOCK), "cancellation worker signals recursive ownership");
  OSReceiveMessage(&data.blocked, nullptr, OS_MESSAGE_BLOCK);
  data.escapedWait = true;
  return nullptr;
}
}

int main() {
  // All raw SDK fields below are observed under the actual cooperative guest
  // CPU scope; blocking SDK calls release it and resume with it held.
  const aurora::os::GuestThreadExecutionScope execution;
  aurora::allocation::routing_state = {true, true};
  Handoff data;
  OSInitMessageQueue(&data.commands, data.commandStorage, 2);
  OSInitMessageQueue(&data.replies, data.replyStorage, 2);
  require(OSCreateThread(&data.thread, handoff, &data, data.stack.data() + data.stack.size(),
                         data.stack.size(), 12, 0), "OSCreateThread succeeds");
  require(OSIsThreadSuspended(&data.thread) && data.thread.suspend == 1, "thread starts suspended exactly once");
  require(!OSIsThreadTerminated(&data.thread), "new thread is live");
  require(OSSendMessage(&data.commands, token(0x1234), OS_MESSAGE_NOBLOCK), "message can be queued before resume");
  for (int i = 0; i < 32; ++i) OSYieldThread();
  require(!data.entered, "initial suspension prevents entry after CPU handoffs");
  require(OSResumeThread(&data.thread) == 1, "first resume reports prior suspension count");
  OSMessage reply = nullptr;
  require(OSReceiveMessage(&data.replies, &reply, OS_MESSAGE_BLOCK) && reply == token(1), "entry handoff reaches main");
  require(OSReceiveMessage(&data.replies, &reply, OS_MESSAGE_BLOCK) && reply == token(0x1234), "exact reply reaches main");
  require(data.thread.state == OS_THREAD_STATE_WAITING, "worker is blocked at the completion queue");
  require(OSSuspendThread(&data.thread) == 0 && OSSuspendThread(&data.thread) == 1, "nested suspend returns actual prior counts");
  require(OSSendMessage(&data.commands, token(0x5678), OS_MESSAGE_NOBLOCK), "suspended worker receives a pending wakeup");
  for (int i = 0; i < 32; ++i) OSYieldThread();
  require(!data.completed, "pending message does not override suspension");
  require(OSResumeThread(&data.thread) == 2 && OSIsThreadSuspended(&data.thread), "one resume preserves remaining suspension");
  for (int i = 0; i < 32; ++i) OSYieldThread();
  require(!data.completed, "nested suspension still prevents execution");
  require(OSResumeThread(&data.thread) == 1, "last resume releases the worker");
  void* returned = nullptr;
  require(OSJoinThread(&data.thread, &returned) && returned == token(0x9876), "join returns the actual entry point result");
  require(data.completed && OSIsThreadTerminated(&data.thread), "joined worker completed and terminated");
  require(!OSJoinThread(&data.thread, nullptr), "a consumed join cannot succeed twice");
  std::puts("PASS: suspended startup, message handoff, nested suspend/resume, join result");
  std::fflush(stdout);

  Cancellation cancelled;
  OSInitMutex(&cancelled.mutex);
  OSInitMessageQueue(&cancelled.ready, cancelled.readyStorage, 1);
  OSInitMessageQueue(&cancelled.blocked, cancelled.blockedStorage, 1);
  require(OSCreateThread(&cancelled.thread, cancellation, &cancelled,
                         cancelled.stack.data() + cancelled.stack.size(), cancelled.stack.size(), 10, 0), "cancellation worker creation succeeds");
  require(OSResumeThread(&cancelled.thread) == 1, "cancellation worker resumes");
  require(OSReceiveMessage(&cancelled.ready, &reply, OS_MESSAGE_BLOCK), "main observes recursive owner readiness");
  require(cancelled.thread.state == OS_THREAD_STATE_WAITING && cancelled.thread.queue == &cancelled.blocked.queueReceive,
          "cancellation targets a real blocked message receiver");
  require(cancelled.mutex.thread == &cancelled.thread && cancelled.mutex.count == 2, "blocked worker owns a recursive mutex twice");
  require(!OSTryLockMutex(&cancelled.mutex), "other thread cannot acquire the live recursive lock");
  OSCancelThread(&cancelled.thread);
  require(!cancelled.escapedWait && OSIsThreadTerminated(&cancelled.thread), "cancel stops the blocked worker before returning");
  require(cancelled.blocked.queueReceive.head == nullptr && cancelled.blocked.queueReceive.tail == nullptr,
          "cancel removes the intrusive queue waiter");
  require(cancelled.mutex.thread == nullptr && cancelled.mutex.count == 0 && cancelled.thread.queueMutex.head == nullptr,
          "cancel releases every recursive lock level and owner link");
  require(OSTryLockMutex(&cancelled.mutex), "released lock can be acquired by the actual main thread");
  OSUnlockMutex(&cancelled.mutex);
  require(OSJoinThread(&cancelled.thread, &returned) && returned == token(~std::uintptr_t{0}), "cancelled join returns original cancellation sentinel");
  std::printf("PASS: cancellation while blocked owning a recursive mutex\nAll %u thread lifecycle checks passed\n", checks);
  aurora::allocation::routing_state = {};
}
