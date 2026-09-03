#include "Game/Camera/CameraParamChunk.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool value) { if (!value) throw std::runtime_error("camera native boundary assertion failed"); }
int main() {
    const float a[4][4]={{1,2,0,3},{0,1,4,5},{2,0,1,6},{0,0,0,1}};
    const float b[4][4]={{2,0,0,7},{0,3,0,11},{0,0,4,13},{0,0,0,1}};
    const float expected[4][4]={{2,6,0,32},{0,3,16,68},{4,0,4,33},{0,0,0,1}};
    for (int alias=0;alias<3;++alias) {
        TProj3f left,right,out;left.set(a);right.set(b);
        TProj3f& result=alias==1?left:alias==2?right:out;
        result.concat(left,right);
        for(int row=0;row<4;++row)for(int col=0;col<4;++col)require(result.mMtx[row][col]==expected[row][col]);
    }
    TPos3f position;
    for(int row=0;row<3;++row)for(int col=0;col<4;++col)position.mMtx[row][col]=90+row*4+col;
    position.identity33();
    for(int row=0;row<3;++row){
        for(int col=0;col<3;++col)require(position.mMtx[row][col]==(row==col?1.0f:0.0f));
        require(position.mMtx[row][3]==93+row*4);
    }
    std::array<u8,64> resource{};
    static_assert(sizeof(intptr_t)==sizeof(void*));
    CameraGeneralParam original,copied;
    original.mNum1=reinterpret_cast<intptr_t>(resource.data());
    copied=original; // The actual complete original operator= body.
    require(reinterpret_cast<u8*>(copied.mNum1)==resource.data());
    if constexpr(sizeof(void*)>4)require(static_cast<std::uintptr_t>(copied.mNum1)>std::numeric_limits<u32>::max());
    for(s32 scalar : {s32(0),s32(-1),std::numeric_limits<s32>::min(),std::numeric_limits<s32>::max()}) {
        original.mNum1=scalar;copied=original;require(copied.mNum1==scalar);
        require(static_cast<s32>(copied.mNum1)==scalar);
    }
    original.mNum1=static_cast<s32>(0x8001FFFFU);
    require(original.getNum1Low()==-32767 && original.getNum1High()==-1);
    original.mNum1=0x7FFF8000;
    require(original.getNum1Low()==32767 && original.getNum1High()==-32768);
    std::cout << "Camera boundaries: actual pointer/copy, signed scalar/packed words, 4x4 alias concat and 3x3 translation preservation pass\n";
}
