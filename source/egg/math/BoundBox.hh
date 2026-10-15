#pragma once

#include "egg/math/Vector.hh"

namespace Kinoko::EGG {

/// @brief A representation of a bounding rectangle.
struct BoundBox2f {
    /// @addr{0x802145C0}
    /// @brief Default constructor
    constexpr BoundBox2f() = default;

    /// @brief Default destructor
    constexpr ~BoundBox2f() = default;

    /// @addr{0x802145F0}
    /// @brief Resets the bounding box
    /// @details Sets the minimum corner to the largest possible value and the maximum corner to the
    /// largest possible negative value. This way, any subsequent point added to the bounding box
    /// will correctly update the minimum and maximum corners.
    constexpr void resetBound() {
        min.set(std::numeric_limits<f32>::max());
        max.set(-std::numeric_limits<f32>::max());
    }

    /// @brief Sets the minimum and maximum corners
    /// @param vMin The new minimum corner
    /// @param vMax The new maximum corner
    constexpr void setDirect(const Vector2f &vMin, const Vector2f &vMax) {
        max = vMax;
        min = vMin;
    }

    /// @brief Sets the minimum corner
    /// @param v The new minimum corner
    constexpr void setMin(const Vector2f &v) {
        min = v;
    }

    /// @brief Sets the maximum corner
    /// @param v The new maximum corner
    constexpr void setMax(const Vector2f &v) {
        max = v;
    }

    Vector2f min; ///< The minimum corner of the bounding box
    Vector2f max; ///< The maximum corner of the bounding box
};

/// @brief A representation of a bounding cuboid.
struct BoundBox3f {
    /// @brief Default constructor
    constexpr BoundBox3f() = default;

    /// @brief Default destructor
    constexpr ~BoundBox3f() = default;

    /// @brief Resets the bounding box
    /// @details Sets the minimum corner to the largest possible value and the maximum corner to the
    /// largest possible negative value. This way, any subsequent point added to the bounding box
    /// will correctly update the minimum and maximum corners.
    constexpr void resetBound() {
        min.set(std::numeric_limits<f32>::max());
        max.set(-std::numeric_limits<f32>::max());
    }

    /// @brief Sets both the minimum and maximum corners to zero
    constexpr void setZero() {
        min.setZero();
        max.setZero();
    }

    /// @brief Sets the minimum and maximum corners
    /// @param vMin The new minimum corner
    /// @param vMax The new maximum corner
    constexpr void setDirect(const Vector3f &vMin, const Vector3f &vMax) {
        max = vMax;
        min = vMin;
    }

    /// @brief Sets the minimum corner
    /// @param v The new minimum corner
    constexpr void setMin(const Vector3f &v) {
        min = v;
    }

    /// @brief Sets the maximum corner
    /// @param v The new maximum corner
    constexpr void setMax(const Vector3f &v) {
        max = v;
    }

    Vector3f min; ///< The minimum corner of the bounding box
    Vector3f max; ///< The maximum corner of the bounding box
};

} // namespace Kinoko::EGG
