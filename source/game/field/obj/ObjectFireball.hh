#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief An individual fireball within an @ref ObjectFirebar or @ref ObjectFireRing
/// @details The owning object sets the fireball's distance from the center of the owning object and
/// sets the angle of the fireball around the owning object's axis of rotation
class ObjectFireball final : public ObjectCollidable {
public:
    /// @addr{0x80768650}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectFireball(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x807689AC}
    /// @brief Default virtual destructor
    ~ObjectFireball() = default;

    /// @addr{0x80768728}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8076871C}
    [[nodiscard]] const char *getKclName() const override {
        return "fireBPlane";
    }

    void setDistance(f32 dist) {
        m_distance = dist;
    }

    void setAngle(f32 angle) {
        m_angle = angle;
    }

    [[nodiscard]] f32 distance() const {
        return m_distance;
    }

    [[nodiscard]] f32 angle() const {
        return m_angle;
    }

private:
    f32 m_distance; ///< Distance from the center of the parent object
    f32 m_angle;    ///< Angle of the fireball about the axis of rotation
};

} // namespace Kinoko::Field
