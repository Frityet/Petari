#include "Game/System/SaveDataBannerCreator.hpp"

#include "Game/LiveActor/Nerve.hpp"

namespace {
NEW_NERVE(SaveDataBannerCreatorNoOperation, SaveDataBannerCreator, NoOperation);
}  // namespace

SaveDataBannerCreator::SaveDataBannerCreator()
    : NerveExecutor("BannerCreator"), mNANDRequestInfo(new NANDRequestInfo()) {
    initNerve(&SaveDataBannerCreatorNoOperation::sInstance);
}

void SaveDataBannerCreator::execute() {
    mNANDRequestInfo->mResult = NAND_RESULT_OK;
    setNerve(&SaveDataBannerCreatorNoOperation::sInstance);
}

bool SaveDataBannerCreator::isDone() const {
    return isNerve(&SaveDataBannerCreatorNoOperation::sInstance);
}

NANDResultCode SaveDataBannerCreator::getResultCode() const {
    return NANDResultCode(mNANDRequestInfo->mResult);
}

void SaveDataBannerCreator::exeNoOperation() {
}
