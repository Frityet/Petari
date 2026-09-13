# Paired-single normalization contract

The nineteenth original Gateway run passes the model heap fix, initializes Mario's real TornadoMario child, and invokes its original appear/reset/calc path before the front direction is populated. Aurora's PSVECNormalize incorrectly asserted nonzero magnitude, copied from the SDK C-vector entry. The actual paired-single SDK implementation is branchless and produces IEEE exceptional results for this input.

Restored the PS operation order: source component snapshots, separately rounded x/y products, fused z-square addition then y addition, existing frsqrte plus one Newton refinement, and GQR0 float-store conversion. Removed only the false numeric precondition; the C implementation retains its original check. No invented zero-vector direction, Game ordering change or actor workaround is added.

Reference source: decomp/src/RVL_SDK/mtx/vec.c PSVECNormalize; actual call stack: ../gateway-wakeup-demo-20260912/original-app-nineteenth-backtrace.log. No component test campaign; next production run is the validation target.

Twenty-ninth production build passed. Twentieth real-disc original run passed the TornadoMario normalization path and reached MarioAnimator initialization. See ../gateway-wakeup-demo-20260912/original-app-twentieth-run.json.
