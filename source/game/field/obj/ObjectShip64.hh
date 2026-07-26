#pragma once

#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the ship on N64 DK's Jungle Parkway
/// @details Follows a rail path, interpolating its forward position along the way to smoothen
/// turns. This class also creates a cylindrical collision object to represent the red paddle
/// wheel's collision shape.
class ObjectShip64 final : public ObjectCollidable {
public:
    ObjectShip64(const System::MapdataGeoObj &params);
    ~ObjectShip64();

    void init() override;
    void calc() override;

    /// @addr{0x80766CA4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x80766BB4}
    [[nodiscard]] const char *getResources() const override {
        return "DKShip64";
    }

    /// @addr{0x80766BC0}
    [[nodiscard]] const char *getKclName() const override {
        return "DKShip64";
    }

    void createCollision() override;
    void calcCollisionTransform() override;
    bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) override;

private:
    /// @addr{0x80766754}
    /// @brief Calculates the smoothed forward direction of the ship along the rail path
    void calcTangent() {
        m_tangent = Interpolate(0.2f, m_tangent, m_railInterpolator->curTangentDir());
        m_tangent.normalise();
        setMatrixFromOrthonormalBasisAndPos(m_tangent);
    }

    EGG::Vector3f m_tangent;                         ///< Smoothed forward direction
    ObjectCollisionCylinder *m_paddleWheelCollision; ///< Collision of the rotating red wheel
};

} // namespace Kinoko::Field
