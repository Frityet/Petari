#include "Game/Util/MathUtil.hpp"
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
void require(bool condition, std::string_view message) { if (!condition) throw std::runtime_error(std::string(message)); }
    void test_vec2_near_zero_boundaries() {
        constexpr auto tolerance = 0.25F;
        const auto outside = std::nextafter(tolerance, 1.0F);
        require(MR::isNearZero(TVec2f{tolerance, tolerance}, tolerance) &&
                    MR::isNearZero(TVec2f{-tolerance, -tolerance}, tolerance),
                "near-zero uses inclusive component bounds, including corners beyond the radial tolerance");
        require(!MR::isNearZero(TVec2f{outside, 0.0F}, tolerance) &&
                    !MR::isNearZero(TVec2f{-outside, 0.0F}, tolerance) &&
                    !MR::isNearZero(TVec2f{0.0F, outside}, tolerance) &&
                    !MR::isNearZero(TVec2f{0.0F, -outside}, tolerance),
                "the next representable component outside either signed bound must fail");
        require(MR::isNearZero(TVec2f{0.0F, -0.0F}, 0.0F) &&
                    !MR::isNearZero(TVec2f{0.0F, 0.0F}, -tolerance),
                "zero tolerance accepts signed zero while negative tolerance retains the original comparisons");
        const auto unordered = std::nanf("");
        require(MR::isNearZero(TVec2f{unordered, 0.0F}, tolerance) &&
                    MR::isNearZero(TVec2f{0.0F, unordered}, tolerance) &&
                    !MR::isNearZero(TVec2f{unordered, outside}, tolerance) &&
                    MR::isNearZero(TVec2f{42.0F, -88.0F}, unordered),
                "unordered comparisons preserve the retail component checks without adding finite-value rejection");
    }
int main(){ test_vec2_near_zero_boundaries(); }
