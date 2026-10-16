#pragma once

#include "egg/math/Math.hh"

#include "egg/util/Stream.hh"

#include <format>

namespace Kinoko::EGG {

/// @brief A 2D float vector.
struct Vector2f {
#ifdef BUILD_DEBUG
    constexpr Vector2f()
        : x(std::numeric_limits<f32>::signaling_NaN()),
          y(std::numeric_limits<f32>::signaling_NaN()) {}
#else
    /// @brief Default non-initializing constructor
    constexpr Vector2f() = default;
#endif

    /// @brief Constructs a vector with the given x and y components
    /// @param x_ The x component of the vector
    /// @param y_ The y component of the vector
    constexpr Vector2f(f32 x_, f32 y_) : x(x_), y(y_) {}

    /// @brief Default destructor
    constexpr ~Vector2f() = default;

    /// @brief Sets both the x and y components to the given value
    /// @param val The value to set both components to
    constexpr void set(f32 val) {
        x = y = val;
    }

    /// @brief Gets a new vector that is the negation of this vector
    /// @return A new vector with both components negated
    [[nodiscard]] constexpr Vector2f operator-() const {
        return Vector2f(-x, -y);
    }

    /// @brief Subtracts another vector from this vector
    /// @param rhs The vector to subtract
    /// @return A new vector that is the result of the subtraction
    [[nodiscard]] constexpr Vector2f operator-(const Vector2f &rhs) const {
        return Vector2f(x - rhs.x, y - rhs.y);
    }

    /// @brief Subtracts another vector from this vector in-place
    /// @param rhs The vector to subtract
    /// @return A reference to this vector after the subtraction
    constexpr Vector2f &operator-=(const Vector2f &rhs) {
        return *this = *this - rhs;
    }

    /// @brief Adds another vector to this vector
    /// @param rhs The vector to add
    /// @return A new vector that is the result of the addition
    [[nodiscard]] constexpr Vector2f operator+(const Vector2f &rhs) const {
        return Vector2f(x + rhs.x, y + rhs.y);
    }

    /// @brief Adds another vector to `this` vector
    /// @param rhs The vector to add
    /// @return A reference to `this` vector after the addition
    constexpr Vector2f &operator+=(const Vector2f &rhs) {
        return *this = *this + rhs;
    }

    /// @brief Multiplies this vector by a scalar
    /// @param scalar The scalar to multiply by
    /// @return A new vector that is the result of the multiplication
    [[nodiscard]] constexpr Vector2f operator*(const f32 scalar) const {
        return Vector2f(x * scalar, y * scalar);
    }

    /// @brief Multiplies this vector by a scalar in-place
    /// @param scalar The scalar to multiply by
    /// @return A reference to `this` vector after the multiplication
    constexpr Vector2f &operator*=(const f32 scalar) {
        return *this = *this * scalar;
    }

    /// @brief Multiplies a scalar by a vector
    /// @param scalar The scalar to multiply
    /// @param rhs The vector to multiply
    /// @return A new vector that is the result of the multiplication
    /// @details This exists so that multiplication can be performed with the scalar on the
    /// left-hand side.
    [[nodiscard]] friend constexpr Vector2f operator*(f32 scalar, const Vector2f &rhs) {
        return Vector2f(scalar * rhs.x, scalar * rhs.y);
    }

    /// @brief Computes the 2D cross product of this vector with another vector
    /// @param rhs The other vector
    /// @return The scalar result of the cross product
    [[nodiscard]] constexpr f32 cross(const Vector2f &rhs) const {
        return x * rhs.y - y * rhs.x;
    }

    /// @brief Computes the dot product of this vector with another vector
    /// @param rhs The other vector
    /// @return The scalar result of the dot product
    [[nodiscard]] constexpr f32 dot(const Vector2f &rhs) const {
        return x * rhs.x + y * rhs.y;
    }

    /// @brief Computes the dot product of this vector with itself
    /// @return The scalar result of the dot product
    [[nodiscard]] constexpr f32 dot() const {
        return x * x + y * y;
    }

    /// @brief Computes the magnitude of the vector
    /// @return The scalar magnitude of the vector
    [[nodiscard]] constexpr f32 length() const {
        return dot() > std::numeric_limits<f32>::epsilon() ? Mathf::sqrt(dot()) : 0.0f;
    }

    /// @addr{0x80243A00}
    /// @brief Normalizes the vector to have a length of 1
    /// @return The original length of the vector before normalization
    /// @details If the vector has a length of zero, then the vector remains unchanged.
    constexpr f32 normalise() {
        f32 len = length();
        if (len != 0.0f) {
            *this = *this * (1.0f / len);
        }

        return len;
    }

    /// @addr{0x80243A78}
    /// @brief Normalizes the vector using a fast inverse square root estimate
    /// @details If the vector has a length near zero, then the vector remains unchanged.
    constexpr void normalise2() {
        f32 sqLen = dot();
        if (sqLen > std::numeric_limits<f32>::epsilon()) {
            *this *= EGG::Mathf::frsqrt(sqLen);
        }
    }

    /// @brief Initializes a Vector2f by reading 8 bytes from the stream
    /// @param stream The input stream to read the vector data from
    void read(Stream &stream) {
        x = stream.read_f32();
        y = stream.read_f32();
    }

    f32 x; ///< X-axis point of the vector
    f32 y; ///< Y-axis point of the vector

    /// @addr{0x80386F78}
    /// @brief Represents the zero vector, `(0.0f, 0.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector2f zero;

    /// @addr{0x80386F80}
    /// @brief Represents the unit vector along the X-axis, `(1.0f, 0.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector2f ex;

    /// @addr{0x80386F88}
    /// @brief Represents the unit vector along the Y-axis, `(0.0f, 1.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector2f ey;
};

inline constexpr Vector2f Vector2f::zero = Vector2f(0.0f, 0.0f);
inline constexpr Vector2f Vector2f::ex = Vector2f(1.0f, 0.0f);
inline constexpr Vector2f Vector2f::ey = Vector2f(0.0f, 1.0f);

/// @brief A 3D float vector.
struct Vector3f {
#ifdef BUILD_DEBUG
    constexpr Vector3f()
        : x(std::numeric_limits<f32>::signaling_NaN()),
          y(std::numeric_limits<f32>::signaling_NaN()),
          z(std::numeric_limits<f32>::signaling_NaN()) {}
#else
    /// @brief Default non-initializing constructor
    constexpr Vector3f() = default;
#endif

    /// @brief Constructs a vector with the given x, y, and z components
    /// @param x_ The x component of the vector
    /// @param y_ The y component of the vector
    /// @param z_ The z component of the vector
    constexpr Vector3f(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}

    /// @brief Default destructor
    constexpr ~Vector3f() = default;

    /// @brief Sets the vector to the zero vector `(0.0f, 0.0f, 0.0f)`
    constexpr void setZero() {
        set(0.0f);
    }

    /// @brief Sets all components of the vector to the given value
    /// @param val The value to set for all components
    constexpr void set(f32 val) {
        x = y = z = val;
    }

    /// @brief Gets a new vector that is the negation of this vector
    /// @return A vector with all components negated
    [[nodiscard]] constexpr Vector3f operator-() const {
        return Vector3f(-x, -y, -z);
    }

    /// @brief Subtracts another vector from this vector
    /// @param rhs The vector to subtract
    /// @return A new vector that is the result of the subtraction
    [[nodiscard]] constexpr Vector3f operator-(const Vector3f &rhs) const {
        return Vector3f(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    /// @brief Subtracts another vector from this vector in-place
    /// @param rhs The vector to subtract
    /// @return A reference to this vector after the subtraction
    constexpr Vector3f &operator-=(const Vector3f &rhs) {
        return *this = *this - rhs;
    }

    /// @brief Adds another vector to this vector
    /// @param rhs The vector to add
    /// @return A new vector that is the result of the addition
    [[nodiscard]] constexpr Vector3f operator+(const Vector3f &rhs) const {
        return Vector3f(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    /// @brief Adds another vector to this vector in-place
    /// @param rhs The vector to add
    /// @return A reference to this vector after the addition
    constexpr Vector3f &operator+=(const Vector3f &rhs) {
        return *this = *this + rhs;
    }

    /// @brief Adds a scalar value to all components of the vector
    /// @param val The scalar value to add
    /// @return A new vector that is the result of the addition
    [[nodiscard]] constexpr Vector3f operator+(f32 val) const {
        return Vector3f(x + val, y + val, z + val);
    }

    /// @brief Adds a scalar value to all components of the vector in-place
    /// @param val The scalar value to add
    /// @return A reference to this vector after the addition
    constexpr Vector3f &operator+=(f32 val) {
        return *this = *this + val;
    }

    /// @brief Multiplies this vector by another vector component-wise
    /// @param rhs The vector to multiply with
    /// @return A new vector that is the result of the multiplication
    [[nodiscard]] constexpr Vector3f operator*(const Vector3f &rhs) const {
        return Vector3f(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    /// @brief Multiplies this vector by a scalar
    /// @param scalar The scalar value to multiply with
    /// @return A new vector that is the result of the multiplication
    [[nodiscard]] constexpr Vector3f operator*(f32 scalar) const {
        return Vector3f(x * scalar, y * scalar, z * scalar);
    }

    /// @brief Multiplies a scalar by a vector
    /// @param scalar The scalar value to multiply with
    /// @param rhs The vector to multiply
    /// @return A new vector that is the result of the multiplication
    /// @details This exists so that multiplication can be performed with the scalar on the
    /// left-hand side.
    [[nodiscard]] friend constexpr Vector3f operator*(f32 scalar, const Vector3f &rhs) {
        return rhs * scalar;
    }

    /// @brief Multiplies this vector by a scalar in-place
    /// @param scalar The scalar value to multiply with
    /// @return A reference to this vector after the multiplication
    constexpr Vector3f &operator*=(f32 scalar) {
        return *this = *this * scalar;
    }

    /// @brief Divides this vector by a scalar
    /// @param scalar The scalar value to divide by
    /// @return A new vector that is the result of the division
    [[nodiscard]] constexpr Vector3f operator/(f32 scalar) const {
        return Vector3f(x / scalar, y / scalar, z / scalar);
    }

    /// @brief Divides this vector by a scalar in-place
    /// @param scalar The scalar value to divide by
    /// @return A reference to this vector after the division
    constexpr Vector3f &operator/=(f32 scalar) {
        return *this = *this / scalar;
    }

    /// @brief Checks if this vector is equal to another vector
    /// @param rhs The vector to compare with
    /// @return `true` if all components of both vectors are equal, `false` otherwise
    [[nodiscard]] constexpr bool operator==(const Vector3f &rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    /// @brief Checks if this vector is not equal to another vector
    /// @param rhs The vector to compare with
    /// @return `true` if any component of both vectors are not equal, `false` otherwise
    [[nodiscard]] constexpr bool operator!=(const Vector3f &rhs) const {
        return !(*this == rhs);
    }

    /// @brief Converts this vector to a string representation
    /// @return A `std::string` representing the vector
    /// @details Allows for copy construction from a vector to a string.
    [[nodiscard]] explicit operator std::string() const {
        return std::format("[0x{:08X}, 0x{:08X}, 0x{:08X}] | [{}, {}, {}]", f2u(x), f2u(y), f2u(z),
                x, y, z);
    }

    /// @addr{0x80214968}
    /// @brief Computes the cross product of this vector with another vector
    /// @param rhs The vector to compute the cross product with
    /// @return A new vector that is the result of the cross product
    [[nodiscard]] constexpr Vector3f cross(const Vector3f &rhs) const {
        return Vector3f(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
    }

    /// @brief The dot product between the vector and itself
    /// @return The squared length of the vector
    [[nodiscard]] constexpr f32 squaredLength() const {
        return x * x + y * y + z * z;
    }

    /// @brief The dot product between two vectors
    /// @param rhs The vector to compute the dot product with
    /// @return The result of the dot product
    [[nodiscard]] constexpr f32 dot(const Vector3f &rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    /// @brief Computes the magnitude of the vector
    /// @return The length of the vector
    [[nodiscard]] constexpr f32 length() const {
        return Mathf::sqrt(squaredLength());
    }

    /// @addr{0x8019AC68}
    /// @brief Computes the magnitude of the vector using paired-singles implementation
    /// @return The length of the vector
    [[nodiscard]] constexpr f32 ps_length() const {
        f32 dot = Mathf::fma(z, z, x * x + y * y);
        return dot == 0.0f ? 0.0f : Mathf::sqrt(dot);
    }

    /// @addr{0x805AEB88}
    /// @brief The projection of this vector onto rhs
    /// @param rhs The vector to project onto
    /// @return The projection of this vector onto rhs
    [[nodiscard]] constexpr Vector3f proj(const Vector3f &rhs) const {
        return rhs * rhs.dot(*this);
    }

    /// @addr{0x805AEBD0}
    /// @brief The rejection of this vector onto rhs
    /// @param rhs The vector to reject from
    /// @return The rejection of this vector onto rhs
    [[nodiscard]] constexpr Vector3f rej(const Vector3f &rhs) const {
        return *this - proj(rhs);
    }

    /// @addr{0x805AEC24}
    /// @brief Computes both the projection and rejection of this vector onto rhs
    /// @param rhs The vector to project and reject onto
    /// @return A `std::pair` containing the projection and rejection of this vector onto rhs
    [[nodiscard]] constexpr std::pair<Vector3f, Vector3f> projAndRej(const Vector3f &rhs) const {
        return std::pair(proj(rhs), rej(rhs));
    }

    /// @brief Returns the absolute value of each element of the vector
    /// @return A vector containing the absolute values of each element
    [[nodiscard]] constexpr Vector3f abs() const {
        return Vector3f(Mathf::abs(x), Mathf::abs(y), Mathf::abs(z));
    }

    /// @brief The square of the distance between two vectors
    /// @param rhs The other vector to compute the distance to
    /// @return The square of the distance between this vector and rhs
    [[nodiscard]] constexpr f32 sqDistance(const Vector3f &rhs) const {
        return (*this - rhs).squaredLength();
    }

    /// @addr{0x806A62A4}
    /// @brief Multiplies a vector by the inverse of val
    /// @param val The value to invert and multiply the vector by
    /// @return The resulting vector after multiplication
    [[nodiscard]] constexpr EGG::Vector3f multInv(f32 val) const {
        return *this * (1.0f / val);
    }

    /// @brief Paired-singles dot product implementation
    /// @return The dot product of the vector with itself
    [[nodiscard]] constexpr f32 ps_dot() const {
        return ps_dot(*this);
    }

    /// @addr{0x8019ACAC}
    /// @brief Paired-singles dot product implementation
    /// @param rhs The other vector to compute the dot product with
    /// @return The dot product of this vector with rhs
    [[nodiscard]] constexpr f32 ps_dot(const Vector3f &rhs) const {
        return Mathf::fma(x, rhs.x, y * rhs.y) + z * rhs.z;
    }

    /// @brief Computes the squared magnitude of the vector using paired-singles
    /// @return The squared magnitude of the vector
    /// @details Differs from ps_dot due to variation in which operands are fused.
    [[nodiscard]] constexpr f32 ps_squareMag() const {
        return Mathf::fma(z, z, x * x) + y * y;
    }

    /// @addr{0x80243ADC}
    /// @brief Normalizes the vector and returns the original length
    /// @return The length of the vector before normalisation.
    constexpr f32 normalise() {
        f32 len = 0.0f;

        if (squaredLength() > std::numeric_limits<f32>::epsilon()) {
            len = length();
            *this *= (1.0f / len);
        }

        return len;
    }

    /// @addr{0x8019AC24}
    /// @brief Computes the normalized vector using paired-singles and the fast inverse square root
    /// approximation
    /// @return The normalized vector
    [[nodiscard]] constexpr EGG::Vector3f ps_normalize() const {
        return *this * EGG::Mathf::frsqrt(ps_squareMag());
    }

    /// @addr{0x80793F04}
    /// @brief Computes the normalized vector and its original magnitude using paired-singles
    /// @return A `std::pair` containing the original magnitude and the normalized vector
    [[nodiscard]] constexpr std::pair<f32, EGG::Vector3f> ps_normalized() {
        f32 mag = ps_length();
        if (mag <= 0.0f) {
            return std::make_pair(mag, EGG::Vector3f::zero);
        }

        return std::make_pair(mag, *this * (1.0f / mag));
    }

    /// @addr{0x80243B6C}
    /// @brief Normalizes the vector using paired-singles and the fast inverse square root
    /// approximation
    constexpr void normalise2() {
        f32 sqLen = squaredLength();
        if (sqLen > std::numeric_limits<f32>::epsilon()) {
            *this *= Mathf::frsqrt(sqLen);
        }
    }

    /// @addr{0x80085580}
    /// @brief Computes a vector whose elements are the max of the elements of both vectors
    /// @param rhs The vector to compare with
    /// @return A vector whose elements are the max of the elements of both vectors
    [[nodiscard]] constexpr Vector3f maximize(const Vector3f &rhs) const {
        Vector3f out;
        out.x = std::max(x, rhs.x);
        out.y = std::max(y, rhs.y);
        out.z = std::max(z, rhs.z);
        return out;
    }

    /// @addr{0x800855C0}
    /// @brief Computes a vector whose elements are the min of the elements of both vectors
    /// @param rhs The vector to compare with
    /// @return A vector whose elements are the min of the elements of both vectors
    [[nodiscard]] constexpr Vector3f minimize(const Vector3f &rhs) const {
        Vector3f out;
        out.x = std::min(x, rhs.x);
        out.y = std::min(y, rhs.y);
        out.z = std::min(z, rhs.z);
        return out;
    }

    /// @addr{0x8019ADE0}
    /// @brief Paired-singles implementation of @ref sqDistance()
    /// @param rhs The vector to compute the squared distance to
    /// @return The squared distance between this vector and rhs
    [[nodiscard]] constexpr f32 ps_sqDistance(const Vector3f &rhs) const {
        return (*this - rhs).ps_dot();
    }

    /// @addr{0x805AE9EC}
    /// @brief Calculates the orthogonal vector, based on the plane defined by this vector and rhs
    /// @param rhs The vector defining the plane with this vector
    /// @param normalise Whether to normalise the resulting orthogonal vector
    /// @return The orthogonal vector within the plane defined by this vector and rhs
    [[nodiscard]] constexpr Vector3f perpInPlane(const EGG::Vector3f &rhs, bool normalise) const {
        if (Mathf::abs(dot(rhs)) == 1.0f) {
            return EGG::Vector3f::zero;
        }

        f32 _x = (rhs.z * x - rhs.x * z) * rhs.z - (rhs.x * y - rhs.y * x) * rhs.y;
        f32 _y = (rhs.x * y - rhs.y * x) * rhs.x - (rhs.y * z - rhs.z * y) * rhs.z;
        f32 _z = (rhs.y * z - rhs.z * y) * rhs.y - (rhs.z * x - rhs.x * z) * rhs.x;

        EGG::Vector3f ret(_x, _y, _z);

        if (normalise) {
            ret.normalise();
        }

        return ret;
    }

    /// @brief Linearly interpolates this vector towards the provided vector with the provided rate
    /// @param t The interpolation rate
    /// @param v The vector to interpolate towards
    /// @return The interpolated vector
    /// @details This function exists in multiple different translation units. Rather than re-define
    /// this function in multiple places, we move the implementation here for Kinoko.
    [[nodiscard]] constexpr EGG::Vector3f interpolate(f32 t, const EGG::Vector3f &v) const {
        return *this + (v - *this) * t;
    }

    /// @brief Initializes a Vector3f by reading 12 bytes from the stream
    /// @param stream The input stream to read the vector data from
    void read(Stream &stream) {
        x = stream.read_f32();
        y = stream.read_f32();
        z = stream.read_f32();
    }

    f32 x; ///< The x component of the vector
    f32 y; ///< The y component of the vector
    f32 z; ///< The z component of the vector

    /// @addr{0x80384BA0}
    /// @brief Represents the zero vector `(0.0f, 0.0f, 0.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector3f zero;

    /// @brief Represents the unit vector with all components set to one `(1.0f, 1.0f, 1.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector3f unit;

    /// @addr{0x80384BB8}
    /// @brief Represents the unit vector along the x-axis `(1.0f, 0.0f, 0.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector3f ex;

    /// @addr{0x80384BD0}
    /// @brief Represents the unit vector along the y-axis `(0.0f, 1.0f, 0.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector3f ey;

    /// @addr{0x80384BE8}
    /// @brief Represents the unit vector along the z-axis `(0.0f, 0.0f, 1.0f)`
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector3f ez;

    /// @addr{0x809C3C04}
    /// @brief Represents a vector with all components set to infinity
    /// @details Since the class definition is incomplete at the time of declaration, we can't
    /// declare as `constexpr` here. Instead, we declare as `static const` and define as
    /// `inline constexpr` outside the class definition.
    static const Vector3f inf;
};

inline constexpr Vector3f Vector3f::zero = Vector3f(0.0f, 0.0f, 0.0f);
inline constexpr Vector3f Vector3f::unit = Vector3f(1.0f, 1.0f, 1.0f);
inline constexpr Vector3f Vector3f::ex = Vector3f(1.0f, 0.0f, 0.0f);
inline constexpr Vector3f Vector3f::ey = Vector3f(0.0f, 1.0f, 0.0f);
inline constexpr Vector3f Vector3f::ez = Vector3f(0.0f, 0.0f, 1.0f);
inline constexpr Vector3f Vector3f::inf = Vector3f(std::numeric_limits<f32>::infinity(),
        std::numeric_limits<f32>::infinity(), std::numeric_limits<f32>::infinity());

} // namespace Kinoko::EGG
