#include "JSystem/JGeometry/TMatrix.hpp"

#include <bit>
#include <cstdint>
#include <iostream>

int main() {
    float fovy, aspect, near, far, x, y;
    while (std::cin >> fovy >> aspect >> near >> far >> x >> y) {
        TProj3f projection, translation;
        projection.makePerspective(fovy, aspect, near, far);
        translation.makeTrans(x, y);
        projection.concat(translation, projection);
        for (const auto& row : projection.mMtx)
            for (float component : row)
                std::cout << std::hex << std::bit_cast<std::uint32_t>(component) << ' ';
        std::cout << '\n';
    }
}
