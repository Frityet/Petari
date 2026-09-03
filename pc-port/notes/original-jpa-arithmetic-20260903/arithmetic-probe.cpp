#include <aurora/ppc_math.hpp>
#include <bit>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1]);
    assert(input);
    char kind;
    size_t conversions = 0, divisions = 0, spheres = 0;
    while (input >> kind) {
        if (kind == 'c') {
            uint32_t bits, expectedWord, expectedHalf;
            int expectedSigned;
            input >> std::hex >> bits >> expectedWord >> expectedHalf >> std::dec >> expectedSigned;
            const float value = std::bit_cast<float>(bits);
            assert(static_cast<uint32_t>(aurora::ppc::truncate_s32(value)) == expectedWord);
            assert(aurora::ppc::truncate_u16(value) == expectedHalf);
            assert(aurora::ppc::truncate_s16(value) == expectedSigned);
            ++conversions;
        } else if (kind == 'd') {
            uint32_t a, b, expected;
            input >> std::hex >> a >> b >> expected >> std::dec;
            assert(static_cast<uint32_t>(aurora::ppc::divide_s32(std::bit_cast<int32_t>(a), std::bit_cast<int32_t>(b))) == expected);
            ++divisions;
        } else if (kind == 's') {
            int32_t angleNum, angleMax, x, div;
            float sweep;
            int expectedPhi, expectedTheta;
            uint32_t expectedIncrement;
            input >> angleNum >> angleMax >> x >> div >> sweep >> expectedPhi >> expectedTheta >> expectedIncrement;
            const auto phi = aurora::ppc::narrow_s16(static_cast<uint32_t>(aurora::ppc::divide_s32(aurora::ppc::shift_left_s32(x, 15), div - 1)) + 0x4000U);
            const float tmp = static_cast<uint16_t>(aurora::ppc::divide_s32(aurora::ppc::shift_left_s32(angleNum, 16), angleMax - 1));
            const auto theta = aurora::ppc::truncate_s16(tmp * sweep + 0x8000);
            assert(phi == expectedPhi && theta == expectedTheta);
            assert(static_cast<uint32_t>(angleNum) + 1 == expectedIncrement);
            ++spheres;
        } else {
            assert(false);
        }
        assert(input);
    }
    std::cout << conversions << " retail fctiwz/sth conversions; " << divisions << " retail divw cases; "
              << spheres << " actual retail sphere arithmetic slices matched\n";
}
