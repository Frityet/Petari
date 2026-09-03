#pragma once

#include "JSystem/JGeometry/TVec.hpp"

namespace JGeometry {
    template <typename T>
    struct TBox {
        T i{};
        T f{};
    };

    template <>
    struct TBox< TVec2< f32 > > {
        f32 getWidth() const {
            return f.x - i.x;
        }
        f32 getHeight() const {
            return f.y - i.y;
        }

        bool isValid() const {
            return f.isAbove(i);
        }

        void addPos(f32 x, f32 y) {
            addPos(TVec2< f32 >(x, y));
        }

        void addPos(const TVec2< f32 >& pos) {
            i.add(pos);
            f.add(pos);
        }

        bool intersect(const TBox< TVec2< f32 > >& other) {
            i.setMax(other.i);
            f.setMin(other.f);
            return isValid();
        }

        TVec2< f32 > i, f;
    };

    template <>
    struct TBox<TVec3<f32>> {
        void extend(const TVec3f &position) {
            if (i.x >= position.x) {
                i.x = position.x;
            }
            if (i.y >= position.y) {
                i.y = position.y;
            }
            if (i.z >= position.z) {
                i.z = position.z;
            }
            if (f.x <= position.x) {
                f.x = position.x;
            }
            if (f.y <= position.y) {
                f.y = position.y;
            }
            if (f.z <= position.z) {
                f.z = position.z;
            }
        }

        [[nodiscard]] bool intersectsPoint(const TVec3f &position) const {
            return position.x >= i.x && position.y >= i.y && position.z >= i.z && position.x < f.x && position.y < f.y && position.z < f.z;
        }

        void extend(const TVec3f &minimum, const TVec3f &maximum) {
            if (i.x >= minimum.x) i.x = minimum.x;
            if (i.y >= minimum.y) i.y = minimum.y;
            if (i.z >= minimum.z) i.z = minimum.z;
            if (f.x <= maximum.x) f.x = maximum.x;
            if (f.y <= maximum.y) f.y = maximum.y;
            if (f.z <= maximum.z) f.z = maximum.z;
        }

        void zero() {
            i.zero();
            f.zero();
        }

        void pad(f32 padding) {
            const TVec3f amount(padding);
            i.sub(amount);
            f.add(amount);
        }

        void getCenter(TVec3f *center) {
            center->lerp(f, i, 0.5F);
        }

        void set(const TVec3f &min, const TVec3f &max) {
            i.set(min);
            f.set(max);
        }

        TVec3f i{};
        TVec3f f{};
    };

    template < typename T >
    struct TBox2 : public TBox< TVec2< T > > {
    public:
        TBox2() {
        }

        TBox2(const TVec2< T >& _i, const TVec2< T >& _f) {
            TBox< TVec2< T > >::i.set(_i);
            TBox< TVec2< T > >::f.set(_f);
        }

        TBox2(T x0, T y0, T x1, T y1) {
            set(x0, y0, x1, y1);
        }
        // void set<T>(const TBox2<T> &a1, const TBox2<T> &a2);

        void operator=(const JGeometry::TBox2< T >&);

        void absolute() {
            if (!this->isValid()) {
                TBox2< T > box(*this);
                this->i.setMin(box.i);
                this->i.setMin(box.f);
                this->f.setMax(box.i);
                this->f.setMax(box.f);
            }
        }

        void set(const TBox< TVec2< T > >& other) {
            set(other.i, other.f);
        }
        void set(const TVec2< T >& i, const TVec2< T >& f) {
            this->i.set(i), this->f.set(f);
        }
        void set(T x0, T y0, T x1, T y1) NO_INLINE {
            this->i.set(x0, y0);
            this->f.set(x1, y1);
        }

        void setInline(T x0, T y0, T x1, T y1) {
            this->i.set(x0, y0);
            this->f.set(x1, y1);
        }

        inline bool intersectsPoint(const TVec2< T >& rPos) const {
            return (rPos.x >= this->i.x && rPos.y >= this->i.y && rPos.x < this->f.x && rPos.y < this->f.y);
        }
    };

    template <typename T>
    struct TBox3 : public TBox<TVec3<T>> {
    };

    template <typename T>
    struct TDirBox3 {
        TVec3<T> _0{};
        TVec3<T> _C{};
        TVec3<T> _18{};
        TVec3<T> _24{};
        TVec3<T> _30{};
    };
}  // namespace JGeometry

using TBox3f = JGeometry::TBox3<f32>;
using TDirBox3f = JGeometry::TDirBox3<f32>;

using TBox2f = JGeometry::TBox2<f32>;
using TBox2s = JGeometry::TBox2<s16>;
