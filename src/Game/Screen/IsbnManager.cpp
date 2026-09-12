#include "Game/Screen/IsbnManager.hpp"
#include <nw4r/db/assert.h>
#include <nw4r/lyt/arcResourceAccessor.h>
#include <nw4r/lyt/layout.h>
#include <nw4r/lyt/textBox.h>
#include <wstring.h>
#include <new>

const char* IsbnManager::cIsbnNumberPaneName[] = {
    "T_number_00", "T_number_01", "T_number_02", "T_number_03", "T_number_04",
};
const char* IsbnManager::cRegistNumberPaneName[] = {
    "T_number_05",
    "T_number_06",
    "T_number_07",
};
const char* IsbnManager::cOtherNumberPaneName[] = {
    "T_number_08",
    "T_number_09",
};

IsbnManager::IsbnManager(MEMAllocator* pAllocator)
    : _0(true), mpAllocator(pAllocator), mpLayout(nullptr), mpResAccessor(nullptr), mDrawInfo(), _68(0), _64(180), _66(180), mpIsbnBuffer(nullptr),
      mpLayoutBuffer(nullptr), mpResAccBuffer(nullptr) {
}

IsbnManager::~IsbnManager() {
}

void IsbnManager::setAdjustRate(f32 x, f32 y) {
    // TODO: Should be `mDrawInfo.SetLocationAdjustScale(nw4r::math::VEC2(x, y))`
    mDrawInfo.mLocationAdjustScale = nw4r::math::VEC2(x, y);
    // TODO: Should be `mDrawInfo.SetLocationAdjust(true);
    mDrawInfo.mFlag.locationAdjust = true;
}

void IsbnManager::setNumber(const wchar_t* pIsbnNumber, const wchar_t* pRegistNumber, const wchar_t* pOtherNumber) {
    if (!(pIsbnNumber)) {
        nw4r::db::Panic("IsbnManager.cpp", 97, "NW4R:Failed assertion pIsbnNumber");
    }
    if (!(pRegistNumber)) {
        nw4r::db::Panic("IsbnManager.cpp", 98, "NW4R:Failed assertion pRegistNumber");
    }
    if (!(pOtherNumber)) {
        nw4r::db::Panic("IsbnManager.cpp", 99, "NW4R:Failed assertion pOtherNumber");
    }
    if (!(mpLayout)) {
        nw4r::db::Panic("IsbnManager.cpp", 100, "NW4R:Failed assertion mpLayout");
    }
    if (!(wcslen( pIsbnNumber ) == 13)) {
        nw4r::db::Panic("IsbnManager.cpp", 102, "NW4R:Failed assertion wcslen( pIsbnNumber ) == 13");
    }
    if (!(wcslen( pRegistNumber ) == 10)) {
        nw4r::db::Panic("IsbnManager.cpp", 103, "NW4R:Failed assertion wcslen( pRegistNumber ) == 10");
    }
    if (!(wcslen( pOtherNumber ) == 7)) {
        nw4r::db::Panic("IsbnManager.cpp", 104, "NW4R:Failed assertion wcslen( pOtherNumber ) == 7");
    }

    const u8 isbnNumberLength[] = {3, 1, 6, 2, 1};
    nw4r::lyt::Pane* pRootPane = mpLayout->mpRootPane;
    for (s32 i = 0; i < 5; i++) {
        nw4r::lyt::TextBox* pTextBox = nw4r::ut::DynamicCast< nw4r::lyt::TextBox* >(pRootPane->FindPaneByName(cIsbnNumberPaneName[i], true));
        u8 length = isbnNumberLength[i];
        pTextBox->SetString(pIsbnNumber, 0, length);
        pIsbnNumber += length;
    }

    const u8 registNumberLength[] = {3, 4, 3};
    for (s32 i = 0; i < 3; i++) {
        nw4r::lyt::TextBox* pTextBox = nw4r::ut::DynamicCast< nw4r::lyt::TextBox* >(pRootPane->FindPaneByName(cRegistNumberPaneName[i], true));
        u8 length = registNumberLength[i];
        pTextBox->SetString(pRegistNumber, 0, length);
        pRegistNumber += length;
    }

    const u8 otherNumberLength[] = {4, 3};
    for (s32 i = 0; i < 2; i++) {
        nw4r::lyt::TextBox* pTextBox = nw4r::ut::DynamicCast< nw4r::lyt::TextBox* >(pRootPane->FindPaneByName(cOtherNumberPaneName[i], true));
        u8 length = otherNumberLength[i];
        pTextBox->SetString(pOtherNumber, 0, length);
        pOtherNumber += length;
    }
}

bool IsbnManager::calc(bool param1) {
    bool result = false;

    if (mpLayout == nullptr) {
        return false;
    }

    if (param1 && _68 > _64) {
        result = true;
    } else if (_68 > _66) {
        result = true;
    }

    calculateView();
    mpLayout->CalculateMtx(mDrawInfo);

    _68++;
    _0 = !result;

    return result;
}

void IsbnManager::draw() {
    if (mpLayout != nullptr) {
        mpLayout->Draw(mDrawInfo);
    }
}

void IsbnManager::reset() {
    _68 = 0;
    _0 = true;
}

void IsbnManager::calculateView() {
    nw4r::math::MTX34 mtx;
    nw4r::ut::Rect rect = mpLayout->GetLayoutRect();

    PSMTXIdentity(mtx);
    mDrawInfo.mViewMtx = mtx;
    mDrawInfo.mViewRect = rect;
}

IsbnManager* IsbnManager::create(void* pArchiveBuf, MEMAllocator* pAllocator) {
    if (!(pArchiveBuf)) {
        nw4r::db::Panic("IsbnManager.cpp", 188, "NW4R:Failed assertion pArchiveBuf");
    }
    if (!(pAllocator)) {
        nw4r::db::Panic("IsbnManager.cpp", 189, "NW4R:Failed assertion pAllocator");
    }

    void* pIsbnBuffer = MEMAllocFromAllocator(pAllocator, sizeof(IsbnManager));
    if (!(pIsbnBuffer)) {
        nw4r::db::Panic("IsbnManager.cpp", 192, "NW4R:Failed assertion pIsbnBuffer");
    }
    IsbnManager* pIsbn = new (pIsbnBuffer) IsbnManager(pAllocator);
    if (!(pIsbn)) {
        nw4r::db::Panic("IsbnManager.cpp", 194, "NW4R:Failed assertion pIsbn");
    }
    pIsbn->mpIsbnBuffer = pIsbnBuffer;

    pIsbn->mpResAccBuffer = MEMAllocFromAllocator(pAllocator, sizeof(nw4r::lyt::ArcResourceAccessor));
    if (!(pIsbn->mpResAccBuffer)) {
        nw4r::db::Panic("IsbnManager.cpp", 199, "NW4R:Failed assertion pIsbn->mpResAccBuffer");
    }
    pIsbn->mpResAccessor = new (pIsbn->mpResAccBuffer) nw4r::lyt::ArcResourceAccessor();
    if (!(pIsbn->mpResAccessor)) {
        nw4r::db::Panic("IsbnManager.cpp", 201, "NW4R:Failed assertion pIsbn->mpResAccessor");
    }

    pIsbn->mpLayoutBuffer = MEMAllocFromAllocator(pAllocator, sizeof(nw4r::lyt::Layout));
    if (!(pIsbn->mpLayoutBuffer)) {
        nw4r::db::Panic("IsbnManager.cpp", 205, "NW4R:Failed assertion pIsbn->mpLayoutBuffer");
    }
    pIsbn->mpLayout = new (pIsbn->mpLayoutBuffer) nw4r::lyt::Layout();
    if (!(pIsbn->mpLayout)) {
        nw4r::db::Panic("IsbnManager.cpp", 207, "NW4R:Failed assertion pIsbn->mpLayout");
    }

    MEMAllocator* pOldAllocator = nw4r::lyt::Layout::mspAllocator;
    nw4r::lyt::Layout::mspAllocator = pAllocator;
    bool result = pIsbn->mpResAccessor->Attach(pArchiveBuf, "./");
    if (!(result)) {
        nw4r::db::Panic("IsbnManager.cpp", 215, "NW4R:Failed assertion result");
    }
    void* lytRes = pIsbn->mpResAccessor->GetResource(0, "IsbnLayout_00.brlyt", nullptr);
    if (!(lytRes)) {
        nw4r::db::Panic("IsbnManager.cpp", 219, "NW4R:Failed assertion lytRes");
    }
    result = pIsbn->mpLayout->Build(lytRes, pIsbn->mpResAccessor);
    if (!(result)) {
        nw4r::db::Panic("IsbnManager.cpp", 221, "NW4R:Failed assertion result");
    }
    nw4r::lyt::Layout::mspAllocator = pOldAllocator;
    return pIsbn;
}
