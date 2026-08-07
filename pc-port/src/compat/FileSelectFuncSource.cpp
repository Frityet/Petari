#include <revolution/types.h>

// Metrowerks builds the retail Game module with a 16-bit wchar_t. Keep the
// byte-exact Game translation unit while giving this one source-shaped bridge
// the same type width and a UTF-16 message entry point on the host.
#define wchar_t u16
#define getGameMessageDirect getGameMessageDirectUtf16
#include "Game/Map/FileSelectFunc.cpp"
#undef getGameMessageDirect
#undef wchar_t
