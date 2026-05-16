#include "game/Game/Util/BitArray.hpp"
#include "tests/TestHarness.hpp"

$test("BitArray stores individual bits across byte boundaries") {
    MR::BitArray bits(17);
    $pc_port_require_eq(bits.size(), 17);

    for (int i = 0; i < bits.size(); ++i) {
        $pc_port_require(!bits.isOn(i));
    }

    bits.set(0, true);
    bits.set(7, true);
    bits.set(8, true);
    bits.set(16, true);

    $pc_port_require(bits.isOn(0));
    $pc_port_require(bits.isOn(7));
    $pc_port_require(bits.isOn(8));
    $pc_port_require(bits.isOn(16));
    $pc_port_require(!bits.isOn(1));
    $pc_port_require(!bits.isOn(15));

    bits.set(7, false);
    bits.set(8, false);

    $pc_port_require(!bits.isOn(7));
    $pc_port_require(!bits.isOn(8));
    $pc_port_require(bits.isOn(0));
    $pc_port_require(bits.isOn(16));
}
