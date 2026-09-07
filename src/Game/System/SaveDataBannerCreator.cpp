#include "Game/System/SaveDataBannerCreator.hpp"

#include <array>
#include <cstdio>
#include <cstring>

#include "Game/LiveActor/Nerve.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/Util/NerveUtil.hpp"

struct NANDBanner {
    std::array<u8, 29344U> bytes{};
};

namespace {
    NEW_NERVE(SaveDataBannerCreatorNoOperation, SaveDataBannerCreator, NoOperation);
    NEW_NERVE(SaveDataBannerCreatorCreateOnTemporary, SaveDataBannerCreator, CreateOnTemporary);
    NEW_NERVE(SaveDataBannerCreatorMoveToHomeDir, SaveDataBannerCreator, MoveToHomeDir);
}  // namespace

SaveDataBannerCreator::SaveDataBannerCreator() : NerveExecutor("BannerCreator"), mNANDRequestInfo(nullptr), mBanner(nullptr), mHomeDir{} {
    mNANDRequestInfo = new NANDRequestInfo();
    initNerve(&SaveDataBannerCreatorNoOperation::sInstance);
    setupBannerInfo();
}

void SaveDataBannerCreator::execute() {
    setNerve(&SaveDataBannerCreatorCreateOnTemporary::sInstance);
}

bool SaveDataBannerCreator::isDone() const {
    return isNerve(&SaveDataBannerCreatorNoOperation::sInstance);
}

NANDResultCode SaveDataBannerCreator::getResultCode() const {
    return NANDResultCode(mNANDRequestInfo->mResult);
}

void SaveDataBannerCreator::exeNoOperation() {
}

void SaveDataBannerCreator::exeCreateOnTemporary() {
    if (MR::isFirstStep(this)) {
        mNANDRequestInfo->setWriteSeq("/tmp/banner.bin", mBanner->bytes.data(), static_cast<u32>(mBanner->bytes.size()), 0x3c, 0);
        MR::addRequestToNANDManager(mNANDRequestInfo);
    }

    if (mNANDRequestInfo->isDone()) {
        const auto result_code = NANDResultCode(mNANDRequestInfo->mResult);
        setNerve(result_code.isSuccess() ? static_cast<const Nerve*>(&SaveDataBannerCreatorMoveToHomeDir::sInstance) :
                                           static_cast<const Nerve*>(&SaveDataBannerCreatorNoOperation::sInstance));
    }
}

void SaveDataBannerCreator::exeMoveToHomeDir() {
    if (MR::isFirstStep(this)) {
        mNANDRequestInfo->setMove("/tmp/banner.bin", mHomeDir);
        MR::addRequestToNANDManager(mNANDRequestInfo);
    }

    if (mNANDRequestInfo->isDone()) {
        setNerve(&SaveDataBannerCreatorNoOperation::sInstance);
    }
}

void SaveDataBannerCreator::setupBannerInfo() {
    mBanner = new NANDBanner();
    std::snprintf(mHomeDir, sizeof(mHomeDir), "%s", "banner.bin");
}
