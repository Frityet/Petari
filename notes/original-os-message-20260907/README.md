# Original OS message queue activation

Aurora now compiles the four complete existing SDK message queue functions from `decomp/src/RVL_SDK/os/OSMessage.c`. The native `.cpp` copy is byte-identical. This closes the first real platform dependency exposed by original JAIStream/manager activation; it is a general queue implementation, using the current actual intrusive OSThread wait queues and Aurora interrupt CPU gate.

Fresh Wii compile succeeds. OSInitMessageQueue, OSSendMessage, OSReceiveMessage and OSJamMessage each compare 100% against the existing RMGK01 retail split object; see `proof.json` and `objdiff.json`. These were already decompiled SDK functions, not newly recovered bodies.

Five new queue tests join the existing thread/mutex executable. All 28 cases pass in ordinary and ThreadSanitizer builds. Coverage includes ring wrap, jam order, null messages and discarded receives; nonblocking full/empty predicates; actual WAITING-thread queue identity; a blocked receiver entered with interrupts disabled and restored disabled after wake; a blocked producer; two wakeups that must recheck an empty queue; explicit protocol shutdown messages and fully drained waiter lists; and 2,000 ordered cross-thread transfers. SDK message queues have no invented close API. Both CMake's existing test target and Aurora's Xmake library source list include the new files.

`verify.py` reruns the native and sanitizer proof independently of root Xmake. Google Test itself is a prebuilt, non-instrumented library. This proves the native cooperative thread/interrupt boundary used by these queues; it does not claim full preemptive Wii scheduling.
