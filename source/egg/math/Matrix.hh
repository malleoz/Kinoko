#pragma once

#include "egg/math/Quat.hh"

namespace Kinoko::EGG {

using namespace Mathf;

/// @brief Represents a 3 x 4 matrix of floating-point numbers
class Matrix34f {
public:
#ifdef BUILD_DEBUG
    constexpr Matrix34f() {
        a.fill(std::numeric_limits<f32>::signaling_NaN());
    }
#else
    /// @brief Default non-initializing constructor
    constexpr Matrix34f() = default;
#endif

    /// @brief Constructs a matrix from individual elements
    /// @param _e00 The element at `[0][0]`
    /// @param _e01 The element at `[0][1]`
    /// @param _e02 The element at `[0][2]`
    /// @param _e03 The element at `[0][3]`
    /// @param _e10 The element at `[1][0]`
    /// @param _e11 The element at `[1][1]`
    /// @param _e12 The element at `[1][2]`
    /// @param _e13 The element at `[1][3]`
    /// @param _e20 The element at `[2][0]`
    /// @param _e21 The element at `[2][1]`
    /// @param _e22 The element at `[2][2]`
    /// @param _e23 The element at `[2][3]`
    constexpr Matrix34f(f32 _e00, f32 _e01, f32 _e02, f32 _e03, f32 _e10, f32 _e11, f32 _e12,
            f32 _e13, f32 _e20, f32 _e21, f32 _e22, f32 _e23)
        : a{{_e00, _e01, _e02, _e03, _e10, _e11, _e12, _e13, _e20, _e21, _e22, _e23}} {}

    /// @brief Default destructor
    constexpr ~Matrix34f() = default;

    /// @brief Compares two matrices for equality
    /// @param rhs The matrix to compare with
    /// @return `true` if the matrices are equal, `false` otherwise
    constexpr bool operator==(const Matrix34f &rhs) const {
        return mtx == rhs.mtx;
    }

    /// @brief Accesses the matrix element at the specified row and column
    /// @param row The row index of the element
    /// @param col The column index of the element
    /// @return A reference to the matrix element at the specified row and column
    [[nodiscard]] constexpr f32 &operator[](size_t row, size_t col) {
        return mtx[row][col];
    }

    /// @brief Accesses the matrix element at the specified row and column
    /// @param row The row index of the element
    /// @param col The column index of the element
    /// @return The matrix element at the specified row and column
    [[nodiscard]] constexpr f32 operator[](size_t row, size_t col) const {
        return mtx[row][col];
    }

    /// @addr{0x80230118}
    /// @brief Sets the matrix from a rotation quaternion and translation/position vector
    /// @param q The rotation quaternion
    /// @param t The translation/position vector
    constexpr void makeQT(const Quatf &q, const Vector3f &t) {
        f32 yy = 2.0f * q.v.y * q.v.y;
        f32 zz = 2.0f * q.v.z * q.v.z;
        f32 xx = 2.0f * q.v.x * q.v.x;
        f32 xy = 2.0f * q.v.x * q.v.y;
        f32 xz = 2.0f * q.v.x * q.v.z;
        f32 yz = 2.0f * q.v.y * q.v.z;
        f32 wz = 2.0f * q.w * q.v.z;
        f32 wx = 2.0f * q.w * q.v.x;
        f32 wy = 2.0f * q.w * q.v.y;

        mtx[0][0] = 1.0f - yy - zz;
        mtx[0][1] = xy - wz;
        mtx[0][2] = xz + wy;

        mtx[1][0] = xy + wz;
        mtx[1][1] = 1.0f - xx - zz;
        mtx[1][2] = yz - wx;

        mtx[2][0] = xz - wy;
        mtx[2][1] = yz + wx;
        mtx[2][2] = 1.0f - xx - yy;

        mtx[0][3] = t.x;
        mtx[1][3] = t.y;
        mtx[2][3] = t.z;
    }

    /// @addr{0x8023030C}
    /// @brief Sets the matrix from a rotation quaternion
    /// @param q The rotation quaternion
    /// @details The translation component of the matrix (column 3) is set to zero.
    constexpr void makeQ(const Quatf &q) {
        f32 yy = 2.0f * q.v.y * q.v.y;
        f32 zz = 2.0f * q.v.z * q.v.z;
        f32 xx = 2.0f * q.v.x * q.v.x;
        f32 xy = 2.0f * q.v.x * q.v.y;
        f32 xz = 2.0f * q.v.x * q.v.z;
        f32 yz = 2.0f * q.v.y * q.v.z;
        f32 wz = 2.0f * q.w * q.v.z;
        f32 wx = 2.0f * q.w * q.v.x;
        f32 wy = 2.0f * q.w * q.v.y;

        mtx[0][0] = 1.0f - yy - zz;
        mtx[0][1] = xy - wz;
        mtx[0][2] = xz + wy;

        mtx[1][0] = xy + wz;
        mtx[1][1] = 1.0f - xx - zz;
        mtx[1][2] = yz - wx;

        mtx[2][0] = xz - wy;
        mtx[2][1] = yz + wx;
        mtx[2][2] = 1.0f - xx - yy;

        mtx[0][3] = 0.0f;
        mtx[1][3] = 0.0f;
        mtx[2][3] = 0.0f;
    }

    /// @addr{0x8022FE14}
    /// @brief Sets the matrix from a rotation vector and a translation/position vector
    /// @param r The rotation vector (Euler angles)
    /// @param t The translation/position vector
    constexpr void makeRT(const Vector3f &r, const Vector3f &t) {
        EGG::Vector3f s = EGG::Vector3f(sin(r.x), sin(r.y), sin(r.z));
        EGG::Vector3f c = EGG::Vector3f(cos(r.x), cos(r.y), cos(r.z));

        const f32 c0_c2 = c.x * c.z;
        const f32 s0_s1 = s.x * s.y;
        const f32 c0_s2 = c.x * s.z;

        mtx[0][0] = (c.y * c.z);
        mtx[1][0] = (c.y * s.z);
        mtx[2][0] = (-s.y);

        mtx[0][1] = (s0_s1 * c.z) - c0_s2;
        mtx[1][1] = (s0_s1 * s.z) + c0_c2;
        mtx[2][1] = (s.x * c.y);

        mtx[0][2] = (c0_c2 * s.y) + (s.x * s.z);
        mtx[1][2] = (c0_s2 * s.y) - (s.x * c.z);
        mtx[2][2] = (c.x * c.y);

        mtx[0][3] = t.x;
        mtx[1][3] = t.y;
        mtx[2][3] = t.z;
    }

    /// @addr{0x8022FF98}
    /// @brief Sets 3x3 rotation matrix from a vector of Euler angles
    /// @param r The rotation vector (Euler angles)
    /// @details The translation/position portion of the matrix is set to zero.
    constexpr void makeR(const Vector3f &r) {
        EGG::Vector3f s = EGG::Vector3f(sin(r.x), sin(r.y), sin(r.z));
        EGG::Vector3f c = EGG::Vector3f(cos(r.x), cos(r.y), cos(r.z));

        const f32 c0_c2 = c.x * c.z;
        const f32 s0_s1 = s.x * s.y;
        const f32 c0_s2 = c.x * s.z;

        mtx[0][0] = (c.y * c.z);
        mtx[1][0] = (c.y * s.z);
        mtx[2][0] = (-s.y);

        mtx[0][1] = (s0_s1 * c.z) - c0_s2;
        mtx[1][1] = (s0_s1 * s.z) + c0_c2;
        mtx[2][1] = (s.x * c.y);

        mtx[0][2] = (c0_c2 * s.y) + (s.x * s.z);
        mtx[1][2] = (c0_s2 * s.y) - (s.x * c.z);
        mtx[2][2] = (c.x * c.y);

        mtx[0][3] = 0.0f;
        mtx[1][3] = 0.0f;
        mtx[2][3] = 0.0f;
    }

    /// @addr{0x80230280}
    /// @brief Sets the matrix based on the provided scale vector
    /// @param s The scale vector
    /// @details The resulting matrix will have the scale values along the diagonal and zeros
    /// elsewhere.
    constexpr void makeS(const Vector3f &s) {
        makeZero();
        mtx[0][0] = s.x;
        mtx[1][1] = s.y;
        mtx[2][2] = s.z;
    }

    /// @addr{0x802302C4}
    /// @brief Sets the matrix based on the provided translation/position vector
    /// @param t The translation/position vector
    /// @details The resulting matrix will have the translation values in the last column and an
    /// identity rotation matrix.
    constexpr void makeT(const Vector3f &t) {
        mtx[0][0] = 1.0f;
        mtx[0][1] = 0.0f;
        mtx[0][2] = 0.0f;
        mtx[1][0] = 0.0f;
        mtx[1][1] = 1.0f;
        mtx[1][2] = 0.0f;
        mtx[2][0] = 0.0f;
        mtx[2][1] = 0.0f;
        mtx[2][2] = 1.0f;
        mtx[0][3] = t.x;
        mtx[1][3] = t.y;
        mtx[2][3] = t.z;
    }

    /// @brief Sets every element of the matrix to zero
    constexpr void makeZero() {
        *this = zero;
    }

    /// @addr{0x805AE7B4}
    /// @brief Sets a 3x3 orthonormal basis for a local coordinate system.
    /// @param forward The forward axis of the local coordinate system.
    /// @param up The reference up axis from an existing coordinate system.
    /// @details In a vector's orthogonal space, there are infinitely many vector pairs orthogonal
    /// to each other. To specify a particular pair and create a local coordinate system, a
    /// reference up vector from an existing coordinate system is provided.
    constexpr void makeOrthonormalBasis(const Vector3f &forward, const Vector3f &up) {
        Vector3f right = up.cross(forward);
        right.normalise();
        Vector3f up_ = forward.cross(right);
        up_.normalise();

        setBase(0, right);
        setBase(1, up_);
        setBase(2, forward);
    }

    /// @addr{0x80537740}
    /// @brief Sets a 3x3 orthonormal basis for a local coordinate system and clears the position
    /// @param forward The forward axis of the local coordinate system
    /// @param up The reference up axis from an existing coordinate system
    /// @details This is similar to @ref makeOrthonormalBasis(), but differs in the use of
    /// paired-singles, the fast inverse square root approximation, and setting the translation
    /// column to zero.
    constexpr void ps_makeOrthonormalBasis(Vector3f forward, Vector3f up) {
        forward.normalise2();
        EGG::Vector3f right = up.cross(forward);
        right.normalise2();
        up = forward.cross(right);

        setBase(3, EGG::Vector3f::zero);
        setBase(0, right);
        setBase(1, up);
        setBase(2, forward);
    }

    /// @addr{0x802303BC}
    /// @brief Rotates the matrix about an axis
    /// @param angle The angle to rotate by, in radians
    /// @param axis The axis to rotate around
    constexpr void setAxisRotation(f32 angle, const EGG::Vector3f &axis) {
        EGG::Quatf q;
        q.setAxisRotation(angle, axis);
        makeQ(q);
    }

    /// @brief Multiplies the first three columns of one row of the matrix by a vector
    /// @param rowIdx The index of the row to multiply
    /// @param row The vector to multiply with the row
    constexpr void mulRow33(size_t rowIdx, const Vector3f &row) {
        mtx[rowIdx][0] *= row.x;
        mtx[rowIdx][1] *= row.y;
        mtx[rowIdx][2] *= row.z;
    }

    /// @brief Sets one column of a matrix
    /// @param col The column to set
    /// @base The new base vector to set for the specified column
    constexpr void setBase(size_t col, const Vector3f &base) {
        mtx[0][col] = base.x;
        mtx[1][col] = base.y;
        mtx[2][col] = base.z;
    }

    /// @addr{0x80230410} @addr{0x80199D64}
    /// @brief Multiplies two matrices using paired-singles
    /// @param rhs The matrix to multiply
    /// @return The result of multiplying this matrix by the `rhs` matrix
    [[nodiscard]] constexpr Matrix34f multiplyTo(const Matrix34f &rhs) const {
        Matrix34f mat;

        mat[0, 0] = fma(rhs[2, 0], mtx[0][2], fma(rhs[1, 0], mtx[0][1], rhs[0, 0] * mtx[0][0]));
        mat[0, 1] = fma(rhs[2, 1], mtx[0][2], fma(rhs[1, 1], mtx[0][1], rhs[0, 1] * mtx[0][0]));
        mat[1, 0] = fma(rhs[2, 0], mtx[1][2], fma(rhs[1, 0], mtx[1][1], rhs[0, 0] * mtx[1][0]));
        mat[1, 1] = fma(rhs[2, 1], mtx[1][2], fma(rhs[1, 1], mtx[1][1], rhs[0, 1] * mtx[1][0]));
        mat[0, 2] = fma(rhs[2, 2], mtx[0][2], fma(rhs[1, 2], mtx[0][1], rhs[0, 2] * mtx[0][0]));
        mat[0, 3] = fma(1.0f, mtx[0][3],
                fma(rhs[2, 3], mtx[0][2], fma(rhs[1, 3], mtx[0][1], rhs[0, 3] * mtx[0][0])));
        mat[1, 2] = fma(rhs[2, 2], mtx[1][2], fma(rhs[1, 2], mtx[1][1], rhs[0, 2] * mtx[1][0]));
        mat[1, 3] = fma(1.0f, mtx[1][3],
                fma(rhs[2, 3], mtx[1][2], fma(rhs[1, 3], mtx[1][1], rhs[0, 3] * mtx[1][0])));
        mat[2, 0] = fma(rhs[2, 0], mtx[2][2], fma(rhs[1, 0], mtx[2][1], rhs[0, 0] * mtx[2][0]));
        mat[2, 1] = fma(rhs[2, 1], mtx[2][2], fma(rhs[1, 1], mtx[2][1], rhs[0, 1] * mtx[2][0]));
        mat[2, 2] = fma(rhs[2, 2], mtx[2][2], fma(rhs[1, 2], mtx[2][1], rhs[0, 2] * mtx[2][0]));
        mat[2, 3] = fma(1.0f, mtx[2][3],
                fma(rhs[2, 3], mtx[2][2], fma(rhs[1, 3], mtx[2][1], rhs[0, 3] * mtx[2][0])));

        return mat;
    }

    /// @brief Multiplies a vector by the 3x4 matrix
    /// @param vec The vector to multiply by the matrix
    /// @return The vector resulting from the multiplication
    [[nodiscard]] constexpr Vector3f multVector(const Vector3f &vec) const {
        Vector3f ret;

        ret.x = mtx[0][0] * vec.x + mtx[0][3] + mtx[0][1] * vec.y + mtx[0][2] * vec.z;
        ret.y = mtx[1][0] * vec.x + mtx[1][3] + mtx[1][1] * vec.y + mtx[1][2] * vec.z;
        ret.z = mtx[2][0] * vec.x + mtx[2][3] + mtx[2][1] * vec.y + mtx[2][2] * vec.z;

        return ret;
    }

    /// @addr{0x802303F8}
    /// @brief Paired-singles implementation of @ref multVector()
    /// @param vec The vector to multiply by the matrix
    /// @return The vector resulting from the multiplication
    [[nodiscard]] constexpr Vector3f ps_multVector(const Vector3f &vec) const {
        Vector3f ret;

        ret.x = fma(mtx[0][2], vec.z, mtx[0][0] * vec.x) + fma(mtx[0][3], 1.0f, mtx[0][1] * vec.y);
        ret.y = fma(mtx[1][2], vec.z, mtx[1][0] * vec.x) + fma(mtx[1][3], 1.0f, mtx[1][1] * vec.y);
        ret.z = fma(mtx[2][2], vec.z, mtx[2][0] * vec.x) + fma(mtx[2][3], 1.0f, mtx[2][1] * vec.y);

        return ret;
    }

    /// @addr{0x8059A4F8}
    /// @brief Multiplies a vector by the 3x3 portion of the matrix
    /// @param vec The vector to multiply by the 3x3 portion of the matrix
    /// @return The vector resulting from the multiplication
    [[nodiscard]] constexpr Vector3f multVector33(const Vector3f &vec) const {
        Vector3f ret;

        ret.x = mtx[0][2] * vec.z + (mtx[0][0] * vec.x + mtx[0][1] * vec.y);
        ret.y = mtx[1][2] * vec.z + (mtx[1][0] * vec.x + mtx[1][1] * vec.y);
        ret.z = mtx[2][2] * vec.z + (mtx[2][0] * vec.x + mtx[2][1] * vec.y);

        return ret;
    }

    /// @addr{0x8067EAEC} @addr{0x8022FB04}
    /// @brief Computes the roll, pitch, and yaw from the current matrix
    /// @return A vector containing the roll, pitch, and yaw represented by this matrix
    [[nodiscard]] constexpr Vector3f calcRPY() const {
        constexpr f32 GIMBAL_LOCK_THRESHOLD = 0.999999f;

        EGG::Vector3f xAxisBasis = base(0);
        f32 absZ = EGG::Mathf::abs(xAxisBasis.z);

        if (absZ > GIMBAL_LOCK_THRESHOLD) {
            f32 y = xAxisBasis.z / absZ * -HALF_PI;
            f32 z = EGG::Mathf::atan2(-mtx[0][1], -xAxisBasis.z * mtx[0][2]);
            return EGG::Vector3f(0.0f, y, z);
        }

        f32 x = EGG::Mathf::atan2(mtx[2][1], mtx[2][2]);
        f32 y = EGG::Mathf::asin(-xAxisBasis.z);
        f32 z = EGG::Mathf::atan2(xAxisBasis.y, xAxisBasis.x);

        return EGG::Vector3f(x, y, z);
    }

    /// @addr{0x8019A970}
    /// @brief Multiplies a vector by the 3x3 portion of the matrix using paired-singles
    /// @param vec The vector to multiply by the 3x3 portion of the matrix
    /// @return The vector resulting from the multiplication
    [[nodiscard]] constexpr Vector3f ps_multVector33(const Vector3f &vec) const {
        Vector3f ret;

        ret.x = fma(mtx[0][2], vec.z, mtx[0][0] * vec.x + mtx[0][1] * vec.y);
        ret.y = fma(mtx[1][2], vec.z, mtx[1][0] * vec.x + mtx[1][1] * vec.y);
        ret.z = fma(mtx[2][2], vec.z, mtx[2][0] * vec.x + mtx[2][1] * vec.y);

        return ret;
    }

    /// @addr{0x8022F90C}
    /// @brief Inverts the 3x3 portion of the 3x4 matrix
    /// @param out Reference to the matrix to store the inverse to
    /// @details Unlike a typical matrix inversion, if the determinant is 0, then this function
    /// returns the identity matrix. Otherwise, the transformation component of the matrix is
    /// unmodified.
    constexpr void inverseTo33(Matrix34f &out) const {
        f32 determinant = ((((mtx[2][1] * (mtx[0][2] * mtx[1][0])) +
                                    ((mtx[2][2] * (mtx[0][0] * mtx[1][1])) +
                                            (mtx[2][0] * (mtx[0][1] * mtx[1][2])))) -
                                   (mtx[0][2] * (mtx[2][0] * mtx[1][1]))) -
                                  (mtx[2][2] * (mtx[1][0] * mtx[0][1]))) -
                (mtx[1][2] * (mtx[0][0] * mtx[2][1]));

        if (determinant == 0.0f) {
            out = ident;
            return;
        }

        f32 invDet = 1.0f / determinant;

        out[0, 2] = (mtx[0][1] * mtx[1][2] - mtx[1][1] * mtx[0][2]) * invDet;
        out[1, 2] = -(mtx[0][0] * mtx[1][2] - mtx[0][2] * mtx[1][0]) * invDet;
        out[2, 1] = -(mtx[0][0] * mtx[2][1] - mtx[2][0] * mtx[0][1]) * invDet;
        out[2, 2] = (mtx[0][0] * mtx[1][1] - mtx[1][0] * mtx[0][1]) * invDet;
        out[2, 0] = (mtx[1][0] * mtx[2][1] - mtx[2][0] * mtx[1][1]) * invDet;
        out[0, 0] = (mtx[1][1] * mtx[2][2] - mtx[2][1] * mtx[1][2]) * invDet;
        out[0, 1] = -(mtx[0][1] * mtx[2][2] - mtx[2][1] * mtx[0][2]) * invDet;
        out[1, 0] = -(mtx[1][0] * mtx[2][2] - mtx[2][0] * mtx[1][2]) * invDet;
        out[1, 1] = (mtx[0][0] * mtx[2][2] - mtx[2][0] * mtx[0][2]) * invDet;
    }

    /// @addr{0x80199FC8}
    /// @brief Invert the 3x4 matrix using paired-singles and a fast reciprocal estimation
    /// @param out Reference to the matrix to store the inverse to
    /// @return Whether or not the matrix was inverted
    /// @details If this function returns `false`, the `out` reference is not modified.
    constexpr bool ps_inverse(Matrix34f &out) const {
        f32 fVar14 = fms(mtx[0][1], mtx[1][2], mtx[1][1] * mtx[0][2]);
        f32 fVar15 = fms(mtx[1][1], mtx[2][2], mtx[2][1] * mtx[1][2]);
        f32 fVar13 = fms(mtx[2][1], mtx[0][2], mtx[0][1] * mtx[2][2]);
        f32 determinant = fma(mtx[2][0], fVar14, fma(mtx[1][0], fVar13, mtx[0][0] * fVar15));

        if (determinant == 0.0f) {
            return false;
        }

        f32 invDet = EGG::Mathf::finv(determinant);

        out[0, 0] = fVar15 * invDet;
        out[0, 1] = fVar13 * invDet;
        out[1, 0] = fms(mtx[1][2], mtx[2][0], mtx[2][2] * mtx[1][0]) * invDet;
        out[1, 1] = fms(mtx[2][2], mtx[0][0], mtx[0][2] * mtx[2][0]) * invDet;
        out[2, 0] = fms(mtx[1][0], mtx[2][1], mtx[1][1] * mtx[2][0]) * invDet;
        out[2, 1] = fms(mtx[0][1], mtx[2][0], mtx[0][0] * mtx[2][1]) * invDet;
        out[2, 2] = fms(mtx[0][0], mtx[1][1], mtx[0][1] * mtx[1][0]) * invDet;
        out[0, 2] = fVar14 * invDet;
        out[0, 3] = -fma(out[0, 2], mtx[2][3], fma(out[0, 1], mtx[1][3], out[0, 0] * mtx[0][3]));
        out[1, 2] = fms(mtx[0][2], mtx[1][0], mtx[1][2] * mtx[0][0]) * invDet;
        out[1, 3] = -fma(out[1, 2], mtx[2][3], fma(out[1, 1], mtx[1][3], out[1, 0] * mtx[0][3]));
        out[2, 3] = -fma(out[2, 2], mtx[2][3], fma(out[2, 1], mtx[1][3], out[2, 0] * mtx[0][3]));

        return true;
    }

    /// @brief Transposes the 3x3 portion of the matrix
    /// @return The transposed 3x3 portion of the matrix
    /// @details The translation column (column 3) remains unchanged from `this` matrix.
    [[nodiscard]] constexpr Matrix34f transpose() const {
        Matrix34f ret = *this;

        ret[0, 1] = mtx[1][0];
        ret[0, 2] = mtx[2][0];
        ret[1, 0] = mtx[0][1];
        ret[1, 2] = mtx[2][1];
        ret[2, 0] = mtx[0][2];
        ret[2, 1] = mtx[1][2];

        return ret;
    }

    /// @addr{0x80537B80}
    /// @brief Get a particular column from a matrix
    /// @param col The index of the column to retrieve
    /// @return The specified column as a @ref Vector3f
    [[nodiscard]] constexpr Vector3f base(size_t col) const {
        return Vector3f(mtx[0][col], mtx[1][col], mtx[2][col]);
    }

    /// @brief Gets the translation vector from the matrix
    /// @return The translation vector (column 3) of the matrix
    [[nodiscard]] constexpr Vector3f translation() const {
        return base(3);
    }

    /// @addr{0x80384370}
    /// @brief The identity matrix
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Matrix34f ident;

    /// @brief A matrix with all elements set to zero
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Matrix34f zero;

private:
    /// @brief A union that allows access both as a 2D array (`mtx`) and as a flat array (`a`)
    union {
        std::array<std::array<f32, 4>, 3> mtx; ///< A 2D array of the matrix elements
        std::array<f32, 12> a;                 ///< A flat array of the matrix elements
    };
};

inline constexpr Matrix34f Matrix34f::ident(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f);
inline constexpr Matrix34f Matrix34f::zero(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f);

} // namespace Kinoko::EGG
