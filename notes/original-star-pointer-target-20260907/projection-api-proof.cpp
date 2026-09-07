#include "JSystem/JGeometry/TMatrix.hpp"
#include "JSystem/JGeometry/TVec.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
int main() {
    Mtx44 source={{2,0,0.25f,0},{0,3,-0.5f,0},{0,0,0.1f,-20},{0,0,-1,0}};
    TProj3f projection;
    projection.setInline(source);
    if (std::memcmp(projection.mMtx,source,sizeof(source)) != 0) return 1;
    TVec3f input(4,5,-10), result;
    projection.mult(input,result);
    if (std::fabs(result.x-0.55f)>1e-6f || result.y!=2.0f || std::fabs(result.z+2.1f)>1e-6f) return 2;
    projection.mult(input,input);
    if (input.x!=result.x || input.y!=result.y || input.z!=result.z) return 3;
    if (TVec2f(1,2).squared(TVec2f(4,6))!=25.0f) return 4;
    std::puts("original projection API and squared-distance checks passed");
}
