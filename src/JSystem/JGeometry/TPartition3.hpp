#pragma once

#include "JSystem/JGeometry/TVec.hpp"

namespace JGeometry {
    template < typename T >
    class TPartition3 {
    public:
        void set(const TVec3< T >& rA, const TVec3< T >& rB, const TVec3< T >& rC) {
            TVec3< T > ab;
            ab.sub(rB, rA);
            TVec3< T > bc;
            bc.sub(rC, rB);
            mNormal.cross(bc, ab);
            mNormal.normalize();
            mDot = mNormal.dot(rA);
        }

        void set(const TVec3< T >& rNormal, const TVec3< T >& rPoint) {
            mNormal.set(rNormal);
            mDot = mNormal.dot(rPoint);
        }

        TVec3< T > mNormal;  // 0x0
        T mDot;              // 0xC
    };
};  // namespace JGeometry
