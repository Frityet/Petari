#include "Game/NPC/MiiFacePartsHolder.hpp"

#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cstdint>
#include <new>

namespace {
    constexpr auto RFL_WORK_ALIGNMENT = std::align_val_t {32U};

    [[nodiscard]] u8 *allocate_rfl_work_buffer() {
        return static_cast<u8 *>(::operator new[](RFLGetWorkSize(FALSE), RFL_WORK_ALIGNMENT));
    }

    void free_rfl_work_buffer(u8 *buffer) noexcept {
        if (buffer != nullptr) {
            ::operator delete[](buffer, RFL_WORK_ALIGNMENT);
        }
    }

    [[nodiscard]] RFLErrcode current_rfl_status() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            return runtime->rfl().async_status();
        }

        return RFLErrcode_NotAvailable;
    }
}  // namespace

MiiFacePartsHolder::MiiFacePartsHolder(int num_parts)
    : LiveActorGroup("Mii顔モデル保持", num_parts), JKRDisposer(), mRFLWorkBuffer(nullptr), _34(nullptr),
      _38(RFLErrcode_NotAvailable), _3C(0) {
}

MiiFacePartsHolder::~MiiFacePartsHolder() {
    RFLExit();
    free_rfl_work_buffer(mRFLWorkBuffer);
}

void MiiFacePartsHolder::init(const JMapInfoIter &) {
    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        _38 = RFLErrcode_NotAvailable;
        return;
    }

    free_rfl_work_buffer(mRFLWorkBuffer);
    mRFLWorkBuffer = allocate_rfl_work_buffer();

    try {
        const auto archive_path = runtime->find_object_archive("MiiFaceDatabase");
        if (!archive_path.has_value()) {
            _38 = RFLInitResAsync(mRFLWorkBuffer, nullptr, 0U, FALSE);
            return;
        }

        auto &archive = runtime->dvd().archive_for_path(*archive_path);
        const auto *resource_entry = archive.find_resource("/RFL_Res.dat");
        if (resource_entry == nullptr) {
            _38 = RFLInitResAsync(mRFLWorkBuffer, nullptr, 0U, FALSE);
            return;
        }

        const auto resource = archive.file_data(*resource_entry);
        _38 = RFLInitResAsync(mRFLWorkBuffer, const_cast<std::uint8_t *>(resource.data()),
                              static_cast<u32>(resource.size()), FALSE);
    } catch (...) {
        _38 = RFLInitResAsync(mRFLWorkBuffer, nullptr, 0U, FALSE);
    }
}

void MiiFacePartsHolder::draw() const {
}

void MiiFacePartsHolder::calcAnim() {
}

void MiiFacePartsHolder::calcViewAndEntry() {
}

void MiiFacePartsHolder::reinitCharModel() {
    _38 = current_rfl_status();
}

bool MiiFacePartsHolder::isInitEnd() const {
    return current_rfl_status() != RFLErrcode_Busy;
}

bool MiiFacePartsHolder::isError() const {
    const auto status = current_rfl_status();
    return status != RFLErrcode_Success && status != RFLErrcode_Busy;
}

MiiFaceParts *MiiFacePartsHolder::createPartsFromReceipe(const char *, const MiiFaceRecipe &) {
    return nullptr;
}

MiiFaceParts *MiiFacePartsHolder::createPartsFromDefault(const char *, u16) {
    return nullptr;
}

void MiiFacePartsHolder::drawEachActor(DrawPartsFuncPtr, const RFLDrawCoreSetting *) const {
}

void MiiFacePartsHolder::drawExtra() const {
}

void MiiFacePartsHolder::setTevOpa() const {
}

void MiiFacePartsHolder::setTevXlu() const {
}
