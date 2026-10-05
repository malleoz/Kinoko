#pragma once

#include "game/field/ObjectCollisionConvexHull.hh"

namespace Kinoko {

namespace Kart {

class KartObject;

} // namespace Kart

namespace Field {

/// @brief Relates a KartObject with its convex hull representation
/// @details Exposes a function to initialize the convex hull based on the KartObject's vehicle.
/// Each vehicle type has a unique set of vertices that define its convex hull. Once initialized,
/// `checkCollision` can be called to check for collisions between the @ref Kart::KartObject and
/// other objects via @ref ObjectDirector.
class ObjectCollisionKart {
public:
    /// @addr{0x8081E0CC}
    /// @brief Constructor
    ObjectCollisionKart() : m_kartObject(nullptr) {}

    /// @addr{0x8081E0E4}
    /// @brief Destructor that destroys the convex hull associated with the @ref Kart::KartObject
    ~ObjectCollisionKart() {
        EGG::egg_delete(m_hull);
    }

    void init(u32 idx);

    size_t checkCollision(const EGG::Matrix34f &mat, const EGG::Vector3f &v);

    [[nodiscard]] static EGG::Vector3f GetHitDirection(u16 idx);
    [[nodiscard]] static constexpr std::span<const EGG::Vector3f> GetVehicleVertices(
            Vehicle vehicle);
    [[nodiscard]] static const EGG::Vector3f &translation(size_t idx);

private:
    ObjectCollisionConvexHull *m_hull; ///< Pointer to the convex hull for the KartObject's vehicle
    Kart::KartObject *m_kartObject;    ///< Pointer to the associated @ref Kart::KartObject
    u32 m_playerIdx;                   ///< Player index in the @ref Kart::KartObjectManager
};

} // namespace Field

} // namespace Kinoko
