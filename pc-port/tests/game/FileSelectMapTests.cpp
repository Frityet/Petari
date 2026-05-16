#include "game/Game/Map/FileSelectFunc.hpp"
#include "game/Game/Map/FileSelectIconID.hpp"
#include "tests/TestHarness.hpp"

$test("FileSelectIconID defaults to Mario fellow icon") {
    FileSelectIconID icon_id;

    $pc_port_require(icon_id.isFellow());
    $pc_port_require(not icon_id.isMii());
    $pc_port_require_eq(icon_id.getFellowID(), FileSelectIconID::Mario);
}

$test("FileSelectIconID copies and compares Mii and fellow ids") {
    FileSelectIconID mii_id;
    mii_id.setMiiIndex(7);

    FileSelectIconID copied_id(mii_id);
    $pc_port_require(copied_id == mii_id);
    $pc_port_require(copied_id.isMii());
    $pc_port_require_eq(copied_id.getMiiIndex(), 7);

    FileSelectIconID fellow_id;
    fellow_id.setFellowID(FileSelectIconID::Peach);

    copied_id.set(fellow_id);
    $pc_port_require(copied_id != mii_id);
    $pc_port_require(copied_id.isFellow());
    $pc_port_require_eq(copied_id.getFellowID(), FileSelectIconID::Peach);
}

$test("FileSelectFunc exposes original Mii name buffer size") {
    $pc_port_require_eq(FileSelectFunc::getMiiNameBufferSize(), 11U);
}

$test("FileSelectFunc clears unavailable host Mii name data") {
    FileSelectIconID mii_id;
    mii_id.setMiiIndex(3);

    u16 name[11]{};
    for (u16& code_unit : name) {
        code_unit = 0xFFFFU;
    }

    FileSelectFunc::copyMiiName(name, mii_id);

    for (const u16 code_unit : name) {
        $pc_port_require_eq(code_unit, 0U);
    }
}
