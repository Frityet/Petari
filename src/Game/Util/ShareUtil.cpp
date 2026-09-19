#include "compat/Cp932Literal.hpp"
#include "Game/Util/ShareUtil.hpp"

ResourceShare::~ResourceShare() {
}

ResourceShare::ResourceShare() : NameObj(CP932("資源共有機構")) {
    _C = new u8[0x80];
    _10 = new u8[0x80];
    _14 = 0;
}
