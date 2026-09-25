from pathlib import Path

vec_donor = Path('decomp/libs/JSystem/include/JSystem/JGeometry/TVec.hpp').read_text()
mtx_donor = Path('decomp/libs/JSystem/include/JSystem/JGeometry/TMatrix.hpp').read_text()

def method(text, signature):
    start = text.index(signature)
    start = text.rfind('\n', 0, start) + 1
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end].replace(' NO_INLINE', '') + '\n\n'

def insert(text, anchor, addition):
    assert text.count(anchor) == 1, (anchor, text.count(anchor))
    return text.replace(anchor, addition + anchor)

p = Path('src/JSystem/JGeometry/TVec.hpp')
s = p.read_text()
s = insert(s, '        void add(const TVec2& value)', method(vec_donor, 'inline bool isZero() const'))
s = insert(s, '        [[nodiscard]] T squareDist(const TVec2& value)', method(vec_donor, 'f32 normalize()'))
s = insert(s, '        constexpr TVec3(f32 value)', '        constexpr TVec3(f32 xz, f32 y) : Vec{xz, y, xz} {\n        }\n\n')
s = insert(s, '        [[nodiscard]] TVec3 multiplyOperatorInline(f32 value)', method(vec_donor, 'inline TVec3 multInLine(f32 val) const') + method(vec_donor, 'inline TVec3 multInLine2(f32 val) const'))
s = insert(s, '        void negate() {', method(vec_donor, 'void orthogonalize2(const TVec3& rKillDir)').replace('JMAVECScaleAdd(rKillDir,', 'JMAVECScaleAdd(&rKillDir,'))
cubic = method(vec_donor, 'void cubic(const TVec3& rP0,')
s = insert(s, '        [[nodiscard]] f32 angle(const TVec3 &value)', '        template <typename T>\n' + cubic + method(vec_donor, 'inline TVec3 copy() const') + method(vec_donor, 'f32 turnRate(const TVec3& rB, f32 maxAngle) const'))
s = insert(s, '        [[nodiscard]] constexpr TVec3 operator/(f32 divisor)', '        [[nodiscard]] TVec3 operator*(const TVec3& value) const {\n            TVec3 result;\n            result.mul(*this, value);\n            return result;\n        }\n\n')
s = s.replace('        void scale(T val);\n', method(vec_donor, 'void scale(T val)').rstrip() + '\n')
s = s.replace('using TVec3s = JGeometry::TVec3<s16>;\n', 'using TVec3s = JGeometry::TVec3<s16>;\nusing TVec3Sc = JGeometry::TVec3<s8>;\n')
p.write_text(s)

p = Path('src/JSystem/JGeometry/TQuat.hpp')
s = p.read_text()
old = '''    struct TQuat4 {
        T x{};
        T y{};
        T z{};
        T w{1};

        TQuat4() = default;
        TQuat4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {
        }
'''
new = '''    struct TQuat4 : public TVec4<T> {
        using TVec4<T>::x;
        using TVec4<T>::y;
        using TVec4<T>::z;
        using TVec4<T>::w;
        using TVec4<T>::set;

        TQuat4() : TVec4<T>(T(0), T(0), T(0), T(1)) {
        }

        TQuat4(T xyz, T w_) : TVec4<T>(xyz, xyz, xyz, w_) {
        }

        TQuat4(T x_, T y_, T z_, T w_) : TVec4<T>(x_, y_, z_, w_) {
        }

        TQuat4(const Quaternion& source) : TVec4<T>(source.x, source.y, source.z, source.w) {
        }
'''
assert old in s
s = s.replace(old, new)
s = insert(s, '        void getEuler(TVec3f& out) const', ''.join(method(vec_donor, 'void setEuler' + axis + '(T _' + axis.lower() + ')') for axis in 'XYZ'))
s = insert(s, '        void rotate(TVec3f& vector) const', method(vec_donor, 'void setRotate(f32 x, f32 y, f32 z, f32 pAngle)'))
s = s.replace('static_assert(sizeof(TQuat4f) == sizeof(f32) * 4U);', 'static_assert(sizeof(TQuat4f) == sizeof(Quaternion));\nstatic_assert(alignof(TQuat4f) == alignof(Quaternion));\nstatic_assert(std::is_standard_layout_v<TQuat4f>);\nstatic_assert(std::is_trivially_copyable_v<TQuat4f>);')
p.write_text(s)

p = Path('src/JSystem/JGeometry/TMatrix.hpp')
s = p.read_text()
s = insert(s, '        void setInline(const SMatrix34C &source)', method(mtx_donor, 'void set(T xx, T xy, T xz, T tx,') + method(mtx_donor, 'void scale(T scale)') + method(mtx_donor, 'inline void scaleInline(T scalar)'))
s = insert(s, '        void concat(const T &lhs, const T &rhs)', method(mtx_donor, 'void scale(f32 scalar)') + method(mtx_donor, 'void scaleXYZ(f32 scalar)'))
s = insert(s, '        inline void getZDir2(TVec3f& rDest)', ''.join(method(mtx_donor, 'void set' + axis + 'Dir(f32 x, f32 y, f32 z)') for axis in 'XYZ') + method(mtx_donor, 'inline void getYDir2(TVec3f& rDest) const') + method(mtx_donor, 'inline void setXYZDir2(const TVec3f& rSrcX,'))
s = insert(s, '        void setRotate(f32 rx, f32 ry, f32 rz)', method(mtx_donor, 'void setEuler(const TVec3f& rRot)'))
s = insert(s, '        void makeTrans(const TVec3f &translation)', method(mtx_donor, 'inline void zeroTransInline2()'))
s = insert(s, '        void makeQuat(const TQuat4f& quaternion)', method(mtx_donor, 'void makeRotate(const TVec3f& rFrom, const TVec3f& rTo, f32 angle)'))
s = insert(s, '        void setQT(const TQuat4f& quaternion, const TVec3f& translation)', method(mtx_donor, 'void setRT(f32 rx, f32 ry, f32 rz, const TVec3f& rSrcTrans)').replace('COS(', 'cos(').replace('SIN(', 'sin(') + method(mtx_donor, 'inline void normalizeBasis()'))
p.write_text(s)

p = Path('src/Game/NameObj/NameObjFactory.cpp')
s = p.read_text()
s = insert(s, '#include "Game/Boss.hpp"', '#include "Game/AreaObj/MercatorTransformCube.hpp"\n')
p.write_text(s)
