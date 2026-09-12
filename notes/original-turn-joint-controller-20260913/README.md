# Original TurnJointCtrl recovery

Recovered the complete TurnJointCtrl TU in decomp first from retail 0x802A1E9C–0x802A28F4, then copied it unchanged into native Game. Its header now declares the real 0x6c Wii object layout and the two 0x18-byte face/waist controller records; native pointers widen naturally. Replaced the old opaque padding, corrected the callback names from udate to update, and restored their actual bool return types.

The implementation uses actual JointController callbacks and JointCtrlRate. It preserves normalized axis selection, separately limited pitch/yaw rotations, original near-distance start/end transitions, spherical target blending, negative rotation angle, original degenerate-vector behavior, and the original blend/update ordering. No fabricated matrix or callback result.

The original five-argument MR::createJointController template was missing from the shared header; added it to reference and native headers. The emitted template, delegated callbacks and vtable match retail 100%. No other existing template was changed.

JointCtrlRate's existing reference update incorrectly used an unsigned countdown and self-division. Retail uses a signed decrement: start rate is (duration-remaining)/duration; end rate is remaining/duration; negative remainder finalizes at 1 or 0. Corrected the reference scalar type and method, then copied the complete currently recovered DynamicJointCtrl source/header native. Zero-duration start/end finalize before division. Unrecovered DynamicJointCtrl node/physics methods were not replaced or invented.

Necessary validation only: both affected Game TUs compile with the original compiler and native compiler. All sixteen controller/helper methods match retail at 98.0–100%; rate update/start/end are 100%, rate constructor 96.875%. Exact per-symbol data in reference-match.json and objdiff snapshots. No new tests or shared build. Root owns final app linking and execution.

New Game TUs auto-glob. MR::extractMtxZDir/vecBlendSphere may be further native link prerequisites, and the imported existing DynamicJointCtrlKeeper methods still reference original unrecovered DynamicJointCtrl methods if those sections are retained. Those remain honest missing providers. Owned shared JointController header hunks are only the five-argument overload; other sources in the ten-file manifest belong to this recovery.

Reference publication: `39fcc64d7785aae3e160ca9685b79ea957821eb4` pushed to `origin/pcp-decomp`, remote SHA verified. The commit contains only the five reference paths in `publication.json`; ShadowVolumeCylinder and NPCUtil.d were excluded. Parent native publication remains with the root agent.
