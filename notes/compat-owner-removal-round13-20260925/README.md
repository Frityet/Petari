# Texture, animation and full-screen blur owner cleanup

Removed six compat files (JutTextureAllocation, MarioAnimatorLifetime and CapturedFrameBlurService source/header pairs), reducing the inventory from 66 to 60.

JUTTexture owns its retained mapped allocation, and CaptureScreenDirector owns its texture directly. MarioAnimator/Xanime owners manage their actual arrays, players, tables and shared upper/lower state; ModelManager retains the completed animator through render-packet lifetime. Original FullScreenBlur.cpp is enabled unchanged from decomp. Its missing hardware behavior is implemented generally in Aurora: scissor offset register decoding, wrapped scissor/viewport coordinates, complete EFB backing, and matching depth-snapshot coordinates.

Validation remains one integrated app build and a fresh-save 120-frame Gateway opening smoke. Both passed with clean shutdown and unchanged binary hash. No focused suites, pixel comparisons or longer gameplay campaign. The restored blur path has not been visually verified; this does not establish rabbit catches or Rosalina spawning.

Per-lane notes/manifests record exact ownership changes. Initial dirty RuntimeContext edits are staged by delta. The initially dirty blur test's actual-process migration remains unstaged; the commit only adapts obsolete API references in its HEAD version. That older standalone fixture still needs its separate lifecycle migration and was not run. No unrelated source/test changes or the six preexisting staged route-note edits were absorbed.
