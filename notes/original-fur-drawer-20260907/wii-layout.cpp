#include "Game/Util/FurDrawer.hpp"
#include <stddef.h>
typedef char Size[sizeof(FurDrawer)==0xFC?1:-1];
typedef char LayerSize[sizeof(FurDrawer::CLayerParam)==12?1:-1];
typedef char O0[offsetof(FurDrawer,mLayerCount)==0xc?1:-1];
typedef char O1[offsetof(FurDrawer,mLength)==0x10?1:-1];
typedef char O2[offsetof(FurDrawer,mIndirect)==0x1c?1:-1];
typedef char O3[offsetof(FurDrawer,mBrightness)==0x28?1:-1];
typedef char O4[offsetof(FurDrawer,mAlpha)==0x34?1:-1];
typedef char O5[offsetof(FurDrawer,mDensity)==0x4c?1:-1];
typedef char O6[offsetof(FurDrawer,mIntensity)==0x5c?1:-1];
typedef char O7[offsetof(FurDrawer,mTransparency)==0x6c?1:-1];
typedef char O8[offsetof(FurDrawer,mOffset)==0x70?1:-1];
typedef char O9[offsetof(FurDrawer,mColor)==0x7c?1:-1];
typedef char O10[offsetof(FurDrawer,mFurMtx)==0x88?1:-1];
typedef char O11[offsetof(FurDrawer,mIndirectMtx)==0xb8?1:-1];
typedef char O12[offsetof(FurDrawer,mFogPosition)==0xe8?1:-1];
typedef char O13[offsetof(FurDrawer,_F8)==0xf8?1:-1];
