#include "compat/StorySequenceExecutorCompileCompat.hpp"

// The decompiler emitted two comparisons between a bool and nullptr.  In the
// original Metrowerks dialect nullptr was the integer literal zero; restore
// that lexical meaning only while compiling the unchanged retail source.
#define nullptr 0
#include "../Game/System/StorySequenceExecutor.cpp"
#undef nullptr
