# Native WiiConnect24 availability

The desktop build has no WiiConnect24/message-board platform service. The original optional NWC24System owner now stays closed under TARGET_PC and starts neither VF nor the original sending worker. Open reports NWC24_ERR_DISABLED through the existing unchanged NWC24Messenger error path; close reports LIB_NOT_OPENED, send rejects the request and isSent stays false with the disabled error. No completion size is invented. The retail branch remains unchanged, and no VF initialization, IOS readiness, user identity, mailbox commit or external sending SDK function is fabricated.

This is an explicit native platform policy at the optional owner boundary, not recovered retail behavior. The actual Messenger, tasks, foreground error UI and background error handling remain original. The unneeded NWC24SendThread translation unit should be excluded from the native source list. No new service, provider or Game-stage-specific condition is introduced.

The existing NandSdkBinding/Aurora NAND service does not supply the FAT-in-NAND mail volume or IOS scheduler. Existing WiiIosService is a synthetic frame model, not a mail owner. Implementing real mail support would require those services and native pointer-width message/work records; it is outside this demo pass.

One direct native compilation passed (native-compile.json/log). No subsystem tests or shared builds were run here; the parent startup build is the next integration check.
