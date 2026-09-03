# Original material and visibility players

AnmPlayer, BtkPlayer, BrkPlayer, BtpPlayer, BpkPlayer and BvaPlayer are now
literal root-source imports under Game/Animation. Their headers match root
as well. AnmPlayer includes the root-first isStop recovery from checkpoint
851dbbaea. The original J3DMaterialTable attachment/removal/texture-matrix
creation functions are imported into J3DMaterialAttachCompat.cpp; existing
constructor/clear/destructor providers remain in J3DModelDataCompat.cpp.

These players operate on real typed objects from the original ResourceHolder.
Frame update, delayed reflectFrame at beginDiff, actual material attachment,
endDiff detachment, stop behavior and shape-packet visibility follow the
original methods. No animation multiplier, actor-specific selection or new
Game divergence is introduced. OriginalResourceHolderTests includes a real
complete model with an authored-name BPK fixture and the actual BpkPlayer/
J3DMaterialAnm calculation path.

verify-animation-imports.py checks all twelve Game source/header copies and
the two SDK function groups (material attachment and J3DSys draw init) against
root. Fourteen comparisons pass. Each of the six new Game translation units
and the SDK attachment translation unit passed an isolated native compile.
Full ResourceHolder integration tests are recorded in the migration README.

ModelManager and MarioAnimator activation still require their complete
material controller/model/scene owner dependencies and direct original J3D
renderer integration. These imports make those dependencies available without
publishing a partial ModelManager or adding a parallel frame controller.
