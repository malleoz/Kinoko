#pragma once

#include "egg/math/Vector.hh"

namespace Kinoko::EGG {

/// @brief A quaternion, used to represent 3D rotation.
/// @details A quaternion is comprised of a scalar (w) and a vector (v).
/// The vector is used to represent the axis of rotation, while
/// the scalar is used to represent the amount of rotation.
struct Quatf {
#ifdef BUILD_DEBUG
    constexpr Quatf() : w(std::numeric_limits<f32>::signaling_NaN()) {}
#else
    /// @brief Default non-initializing constructor
    constexpr Quatf() = default;
#endif

    /// @brief Constructs a quaternion from a scalar and a vector
    /// @param w_ The scalar component of the quaternion
    /// @param v_ The vector component of the quaternion
    constexpr Quatf(f32 w_, const Vector3f &v_) : v(v_), w(w_) {}

    /// @brief Constructs a quaternion from a scalar and individual vector components
    /// @param w_ The scalar component of the quaternion
    /// @param x_ The x component of the vector
    /// @param y_ The y component of the vector
    /// @param z_ The z component of the vector
    constexpr Quatf(f32 w_, f32 x_, f32 y_, f32 z_) : v(x_, y_, z_), w(w_) {}

    /// @brief Default destructor
    constexpr ~Quatf() = default;

    /// @brief Adds another quaternion to this quaternion
    /// @param rhs The quaternion to add
    /// @return The result of the addition
    [[nodiscard]] constexpr Quatf operator+(const Quatf &rhs) const {
        return Quatf(w + rhs.w, v + rhs.v);
    }

    /// @brief Adds another quaternion to this quaternion in-place
    /// @param rhs The quaternion to add
    /// @return A reference to this quaternion after the addition
    constexpr Quatf &operator+=(const Quatf &rhs) {
        return *this = *this + rhs;
    }

    /// @brief Multiplies a quaternion by a vector
    /// @param vec The vector to multiply with this quaternion
    /// @return The quaternion resulting from the multiplication
    /// @details To multiply a quaternion by a vector, the vector is treated as a quaternion with a
    /// zero scalar component. Thus, this is just a simplified version of @ref operator*(const Quatf
    /// &) const.
    [[nodiscard]] constexpr Quatf operator*(const Vector3f &vec) const {
        Vector3f cross = v.cross(vec);
        Vector3f scale = vec * w;
        return Quatf(-v.dot(vec), cross + scale);
    }

    /// @brief Multiplies this quaternion by a scalar
    /// @param scalar The scalar value to multiply with this quaternion
    /// @return The quaternion resulting from the multiplication
    [[nodiscard]] constexpr Quatf operator*(f32 scalar) const {
        return Quatf(w * scalar, v * scalar);
    }

    /// @brief Multiplies this quaternion by a scalar in-place
    /// @param scalar The scalar value to multiply with this quaternion
    /// @return A reference to this quaternion after the multiplication
    constexpr Quatf &operator*=(f32 scalar) {
        return *this = *this * scalar;
    }

    /// @brief Multiplies this quaternion by another quaternion
    /// @param rhs The quaternion to multiply with this quaternion
    /// @return The quaternion resulting from the multiplication
    /// @details Computes the resulting quaternion as follows:
    /// \f[\begin{aligned}
    /// w &= w_1 w_2 - \mathbf{v}_1 \cdot \mathbf{v}_2 \\
    /// \mathbf{v} &= w_1 \mathbf{v}_2 + w_2 \mathbf{v}_1 + \mathbf{v}_1 \times \mathbf{v}_2
    /// \end{aligned}\f]
    /// @note Though part of this is a vector cross/dot product, FP arithmetic is not associative or
    /// commutative. It has to be done in this order.
    [[nodiscard]] constexpr Quatf operator*(const Quatf &rhs) const {
        f32 _w = w * rhs.w - v.x * rhs.v.x - v.y * rhs.v.y - v.z * rhs.v.z;
        f32 _x = v.y * rhs.v.z + (v.x * rhs.w + w * rhs.v.x) - v.z * rhs.v.y;
        f32 _y = v.z * rhs.v.x + (v.y * rhs.w + w * rhs.v.y) - v.x * rhs.v.z;
        f32 _z = v.x * rhs.v.y + (v.z * rhs.w + w * rhs.v.z) - v.y * rhs.v.x;

        return Quatf(_w, _x, _y, _z);
    }

    /// @brief Multiplies this quaternion by another quaternion in-place
    /// @param q The quaternion to multiply with this quaternion
    /// @return A reference to this quaternion after the multiplication
    constexpr Quatf &operator*=(const Quatf &q) {
        return *this = *this * q;
    }

    /// @brief Checks if this quaternion is equal to another quaternion
    /// @param rhs The quaternion to compare with this quaternion
    /// @return `true` if the scalar and vector components of both quaternions are equal, `false`
    /// otherwise
    [[nodiscard]] constexpr bool operator==(const Quatf &rhs) const {
        return w == rhs.w && v == rhs.v;
    }

    /// @brief Checks if this quaternion is not equal to another quaternion
    /// @param rhs The quaternion to compare with this quaternion
    /// @return `true` if any of the scalar or vector components of both quaternions are not equal,
    /// `false` otherwise
    [[nodiscard]] constexpr bool operator!=(const Quatf &rhs) const {
        return !(*this == rhs);
    }

    /// @brief Converts this quaternion to a string representation
    /// @return A `std::string` representing the quaternion
    /// @details Allows for copy construction from a quaternion to a string.
    [[nodiscard]] explicit operator std::string() const {
        return std::format("[0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}] | [{}, {}, {}, {}]", f2u(v.x),
                f2u(v.y), f2u(v.z), f2u(w), v.x, v.y, v.z, w);
    }

    /// @addr{0x80239E10}
    /// @brief Sets roll, pitch, and yaw from the provided vector
    /// @param rpy A vector containing the roll, pitch, and yaw angles (in radians)
    constexpr void setRPY(const Vector3f &rpy) {
        *this = FromRPY(rpy.x, rpy.y, rpy.z);
    }

    /// @brief Sets roll, pitch, and yaw from the provided arguments
    /// @param r The roll to set
    /// @param p The pitch to set
    /// @param y The yaw to set
    /// @details This exists to avoid unnecessary Vector3f construction.
    constexpr void setRPY(f32 r, f32 p, f32 y) {
        const f32 cy = Mathf::cos(y * 0.5f);
        const f32 cp = Mathf::cos(p * 0.5f);
        const f32 cr = Mathf::cos(r * 0.5f);
        const f32 sy = Mathf::sin(y * 0.5f);
        const f32 sp = Mathf::sin(p * 0.5f);
        const f32 sr = Mathf::sin(r * 0.5f);

        w = cy * cp * cr + sy * sp * sr;
        v.x = cy * cp * sr - sy * sp * cr;
        v.y = cy * sp * cr + sy * cp * sr;
        v.z = sy * cp * cr - cy * sp * sr;
    }

    /// @addr{0x8023A168}
    /// @brief Scales the quaternion to a unit length
    constexpr void normalise() {
        f32 len = squaredNorm() > std::numeric_limits<f32>::epsilon() ? norm() : 0.0f;

        if (len != 0.0f) {
            f32 inv = 1.0f / len;
            w *= inv;
            v *= inv;
        }
    }

    /// @addr{0x8023A788}
    /// @brief Computes the quaternion representing the rotation from the `from` vector to the `to`
    /// vector
    /// @param from The starting vector
    /// @param to The ending vector
    constexpr void makeVectorRotation(const Vector3f &from, const Vector3f &to) {
        f32 t0 = std::max(0.0f, (from.dot(to) + 1) * 2.0f);
        t0 = Mathf::sqrt(t0);

        if (t0 <= std::numeric_limits<f32>::epsilon()) {
            *this = Quatf::ident;
        } else {
            const f32 inv = 1.0f / t0;
            w = t0 * 0.5f;
            v = from.cross(to) * inv;
        }
    }

    /// @brief Computes the conjugate of the quaternion
    /// @return The conjugate of the quaternion
    /// @details The conjugate is simply the negation of all vector components.
    [[nodiscard]] constexpr Quatf conjugate() const {
        return Quatf(w, -v);
    }

    /// @addr{0x8023A2D0}
    /// @brief Rotates a vector based on the quaternion
    /// @param vec The vector to rotate
    /// @return The rotated vector
    [[nodiscard]] constexpr Vector3f rotateVector(const Vector3f &vec) const {
        Quatf conj = conjugate();
        Quatf res = *this * vec;
        Quatf ret;

        ret.v.x = (res.v.y * conj.v.z + (res.v.x * conj.w + res.w * conj.v.x)) - res.v.z * conj.v.y;
        ret.v.y = (res.v.z * conj.v.x + (res.v.y * conj.w + res.w * conj.v.y)) - res.v.x * conj.v.z;
        ret.v.z = (res.v.x * conj.v.y + (res.v.z * conj.w + res.w * conj.v.z)) - res.v.y * conj.v.x;

        return ret.v;
    }

    /// @addr{0x8023A404}
    /// @brief Rotates a vector based on the inverse of the quaternion
    /// @param vec The vector to rotate
    /// @return The rotated vector
    [[nodiscard]] constexpr Vector3f rotateVectorInv(const Vector3f &vec) const {
        Quatf conj = conjugate();
        Quatf res = conj * vec;
        Quatf ret;

        ret.v.x = (res.v.y * v.z + (res.v.x * w + res.w * v.x)) - res.v.z * v.y;
        ret.v.y = (res.v.z * v.x + (res.v.y * w + res.w * v.y)) - res.v.x * v.z;
        ret.v.z = (res.v.x * v.y + (res.v.z * w + res.w * v.z)) - res.v.y * v.x;

        return ret.v;
    }

    /// @addr{0x8023A5C4}
    /// @brief Performs spherical linear interpolation
    /// @param q1 The quaternion to rotate towards
    /// @param t The interpolation factor
    /// @return The interpolated quaternion
    /// @details Slerp is a method in which you can create smooth rotations between two quaternions.
    [[nodiscard]] constexpr Quatf slerpTo(const Quatf &q1, f32 t) const {
        constexpr f32 EPSILON = 0.00001f;

        f32 dot_ = std::clamp(dot(q1), -1.0f, 1.0f);
        bool bDot = dot_ < 0.0f;
        dot_ = Mathf::abs(dot_);

        f32 acos = Mathf::acos(dot_);
        f32 sin = Mathf::sin(acos);

        f32 s;
        if (Mathf::abs(sin) < EPSILON) {
            s = 1.0f - t;
        } else {
            f32 invSin = 1.0f / sin;
            f32 tmp0 = t * acos;
            s = invSin * Mathf::sin(acos - tmp0);
            t = invSin * Mathf::sin(tmp0);
        }

        if (bDot) {
            t = -t;
        }

        return Quatf(s * w + t * q1.w, s * v + t * q1.v);
    }

    /// @addr{0x8023A138}
    /// @brief Computes the squared magnitude of the quaternion
    /// @return The square magnitude of the quaternion
    /// @details Computes \f$this \cdot this = w^2 + x^2 + y^2 + z^2\f$
    [[nodiscard]] constexpr f32 squaredNorm() const {
        return w * w + v.squaredLength();
    }

    /// @brief Computes the magnitude of the quaternion
    /// @return The magnitude of the quaternion
    [[nodiscard]] constexpr f32 norm() const {
        return Mathf::sqrt(squaredNorm());
    }

    /// @brief Computes the dot product of this quaternion with another quaternion
    /// @param q The other quaternion
    /// @return The dot product of the two quaternions
    /// @details Computes \f$this \cdot rhs = w \times rhs.w + x \times rhs.x + y \times rhs.y + z
    /// \times rhs.z\f$
    [[nodiscard]] constexpr f32 dot(const Quatf &q) const {
        return w * q.w + v.dot(q.v);
    }

    /// @addr{0x8023A0A0}
    /// @brief Sets the quaternion from a given angle and axis
    /// @param angle The rotation angle in radians
    /// @param axis The rotation axis
    constexpr void setAxisRotation(f32 angle, const EGG::Vector3f &axis) {
        const f32 half_angle = angle * 0.5f;
        w = Mathf::cos(half_angle);
        v = axis * Mathf::sin(half_angle);
    }

    /// @brief Multiplies a quaternion by a vector
    /// @param vec The vector to multiply with this quaternion
    /// @return The quaternion resulting from the multiplication
    /// @details This differs from @ref operator*(const Vector3f &) const in that the order of
    /// multiplication is swapped. This is implemented in order to match base game behavior.
    [[nodiscard]] constexpr Quatf multSwap(const Vector3f &vec) const {
        f32 _w = -(v.dot(vec));
        f32 _x = (w * vec.x + v.y * vec.z) - v.z * vec.y;
        f32 _y = (w * vec.y + v.z * vec.x) - v.x * vec.z;
        f32 _z = (w * vec.z + v.x * vec.y) - v.y * vec.x;

        return Quatf(_w, _x, _y, _z);
    }

    /// @brief Multiplies a quaternion by another quaternion
    /// @param q The quaternion to multiply with this quaternion
    /// @return The quaternion resulting from the multiplication
    /// @details This differs from @ref operator*(const Quatf &) const in that the order of
    /// multiplication is swapped. This is implemented in order to match base game behavior.
    [[nodiscard]] constexpr Quatf multSwap(const Quatf &q) const {
        f32 _w = ((w * q.w - v.x * q.v.x) - v.y * q.v.y) - v.z * q.v.z;
        f32 _x = (v.y * q.v.z + (v.x * q.w + w * q.v.x)) - v.z * q.v.y;
        f32 _y = (v.z * q.v.x + (v.y * q.w + w * q.v.y)) - v.x * q.v.z;
        f32 _z = (v.x * q.v.y + (v.z * q.w + w * q.v.z)) - v.y * q.v.x;

        return Quatf(_w, _x, _y, _z);
    }

    /// @brief Reads the quaternion from a stream
    /// @param stream The input stream to read from
    /// @details Reads the quaternion components from the stream, with the vector part first
    /// followed by the scalar part.
    void read(Stream &stream) {
        v.read(stream);
        w = stream.read_f32();
    }

    /// @brief Constructs a quaternion from roll, pitch, and yaw angles
    /// @param rpy The vector containing roll, pitch, and yaw angles
    /// @return The constructed quaternion
    [[nodiscard]] static constexpr Quatf FromRPY(const EGG::Vector3f &rpy) {
        Quatf ret;
        ret.setRPY(rpy);
        return ret;
    }

    /// @brief Constructs a quaternion from roll, pitch, and yaw arguments
    /// @param r The roll angle in radians
    /// @param p The pitch angle in radians
    /// @param y The yaw angle in radians
    /// @return The constructed quaternion
    /// @details This exists to avoid unnecessary Vector3f construction.
    [[nodiscard]] static constexpr Quatf FromRPY(f32 r, f32 p, f32 y) {
        Quatf ret;
        ret.setRPY(r, p, y);
        return ret;
    }

    Vector3f v; ///< The vector component of the quaternion
    f32 w;      ///< The scalar component of the quaternion

    /// @brief The identity quaternion, `(0.0f, 0.0f, 0.0f, 1.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Quatf ident;
};

inline constexpr Quatf Quatf::ident = Quatf(1.0f, Vector3f::zero);

} // namespace Kinoko::EGG
