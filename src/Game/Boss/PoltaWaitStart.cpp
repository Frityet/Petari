#include "resource/TextEncoding.hpp"
#include "Game/Boss/PoltaWaitStart.hpp"
#include "Game/Boss/Polta.hpp"
#include "Game/Boss/PoltaActionBase.hpp"
#include "Game/Boss/PoltaFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"

PoltaWaitStart::PoltaWaitStart(Polta* pPolta) : PoltaActionBase(CP932("ポルタ開始待ち"), pPolta) {
}

void PoltaWaitStart::appear() {
    mIsDead = false;
    PoltaFunction::killLeftArm(getHost());
    PoltaFunction::killRightArm(getHost());
    MR::hideModel(getHost());
}

PoltaWaitStart::~PoltaWaitStart() {
}
