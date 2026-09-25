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
    /// @addr{0x80765C94}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    ObjectShip64(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x80765DB0}
    /// @brief Virtual destructor that also deletes the paddle wheel collision object
    ~ObjectShip64() override {
        EGG::egg_delete(m_paddleWheelCollision);
    }

    /// @addr{0x80765E30}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the rail interpolator to the beginning of the rail and sets @ref
    /// m_tangent based on the rail interpolator's tangent direction. Calls @ref calc() to update
    /// the ship once. Lastly, calls @ref calcModel() to update the ship's transformation matrix.
    void init() override {
        m_railInterpolator->init(0, 0);
        m_tangent = m_railInterpolator->curTangentDir();
        m_railInterpolator->setPerPointVelocities(true);
        calc();
        calcModel();
    }

    /// @addr{0x80766144}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the rail interpolator and updates the ship's position accordingly.
    /// Calls @ref calcTangent() to interpolate the forward direction to create smooth movement
    /// along the rail.
    void calc() override {
        m_railInterpolator->calc();
        setPos(m_railInterpolator->curPos());
        calcTangent();
    }

    /// @addr{0x80766CA4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80766BB4}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name for the ship.
    /// @return The resource name for the ship (`DKShip64`).
    [[nodiscard]] const char *getResources() const override {
        return "DKShip64";
    }

    /// @addr{0x80766BC0}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the ship (`DKShip64`).
    [[nodiscard]] const char *getKclName() const override {
        return "DKShip64";
    }

    /// @addr{0x80766864}
    /// @copybrief ObjectBase::createCollision()
    /// @details Creates the primary and paddle wheel collision objects for the ship.
    void createCollision() override {
        constexpr f32 RADIUS = 1500.0f;
        constexpr f32 HEIGHT = 3500.0f;

        ObjectCollidable::createCollision();
        m_paddleWheelCollision =
                EGG::egg_new<ObjectCollisionCylinder>(RADIUS, HEIGHT, EGG::Vector3f::zero);
    }

    /// @addr{0x807668D4}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details Calculates the transformation matrices for the primary and paddle wheel collision
    /// objects based on the ship's current orientation.
    void calcCollisionTransform() override {
        ObjectCollidable::calcCollisionTransform();
        calcTransform();
        EGG::Matrix34f mat = transform();

        EGG::Vector3f v;
        v = mat.base(0);
        v.normalise();

        mat.setAxisRotation(HALF_PI, v);
        mat.setBase(3, v);
        m_paddleWheelCollision->transform(mat, scale());
    }

    /// @addr{0x80766BCC}
    /// @copydoc ObjectCollidable::checkCollision()
    /// @details Checks for collision against both the boat and the paddle wheel.
    bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) override {
        EGG::Vector3f colDist = EGG::Vector3f::zero;
        EGG::Vector3f auxDist = EGG::Vector3f::zero;

        bool has_col =
                lhs->check(*m_collision, colDist) || lhs->check(*m_paddleWheelCollision, auxDist);
        dist = colDist + auxDist;

        return has_col;
    }

private:
    /// @addr{0x80766754}
    /// @brief Calculates the smoothed forward direction of the ship along the rail path
    /// @details Interpolates @ref m_tangent towards the rail interpolator's tangent direction with
    /// an interpolation rate of `0.2f`. Updates the ship's transformation matrix accordingly.
    void calcTangent() {
        constexpr f32 INTERP_RATE = 0.2f;

        m_tangent = Interpolate(INTERP_RATE, m_tangent, m_railInterpolator->curTangentDir());
        m_tangent.normalise();
        setMatrixFromOrthonormalBasisAndPos(m_tangent);
    }

    EGG::Vector3f m_tangent;                         ///< Smoothed forward direction
    ObjectCollisionCylinder *m_paddleWheelCollision; ///< Collision of the rotating red wheel
};

} // namespace Kinoko::Field
