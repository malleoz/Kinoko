#pragma once

#include "egg/math/Quat.hh"

namespace EGG {

/// @brief A 3 x 4 matrix.
class Matrix34f {
public:
    Matrix34f();
    Matrix34f(f32 _e00, f32 _e01, f32 _e02, f32 _e03, f32 _e10, f32 _e11, f32 _e12, f32 _e13,
            f32 _e20, f32 _e21, f32 _e22, f32 _e23);
    ~Matrix34f();

    bool operator==(const Matrix34f &rhs) const {
        return mtx == rhs.mtx;
    }

    /// @brief Accesses the matrix element at the specified row and column.
    f32 &operator[](size_t row, size_t col) {
        return mtx[row][col];
    }

    /// @brief Accesses the matrix element at the specified row and column.
    f32 operator[](size_t row, size_t col) const {
        return mtx[row][col];
    }

    void makeQT(const Quatf &q, const Vector3f &t);    ///< Sets matrix from rotation and position.
    void makeQ(const Quatf &q);                        ///< Sets rotation matrix from quaternion.
    void makeRT(const Vector3f &r, const Vector3f &t); ///< Sets rotation-translation matrix.
    void makeR(const Vector3f &r); ///< Sets 3x3 rotation matrix from a vector of Euler angles.
    void makeZero();               ///< Zeroes every element of the matrix.
    void setAxisRotation(f32 angle, const Vector3f &axis); ///< Rotates the matrix about an axis.

    Matrix34f multiplyTo(const Matrix34f &rhs) const;  ///< Multiplies two matrices.
    Vector3f multVector(const Vector3f &vec) const;    ///< Multiplies a vector by a matrix.
    Vector3f ps_multVector(const Vector3f &vec) const; ///< Paired-singles impl. of @ref multVector.
    Vector3f multVector33(const Vector3f &vec) const;  ///< Multiplies a 3x3 matrix by a vector.
    Matrix34f inverseTo() const;                       ///< Computes the inverse of the matrix.
    Matrix34f transpose() const; ///< Transposes the 3x3 portion of the matrix.

    static const Matrix34f ident;
    static const Matrix34f zero;

private:
    union {
        std::array<std::array<f32, 4>, 3> mtx;
        std::array<f32, 12> a;
    };
};

} // namespace EGG
