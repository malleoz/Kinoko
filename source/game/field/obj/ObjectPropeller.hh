#pragma once

#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Rotating koopa shell lasers on Koopa Cape
/// @details The shell itself has cylindrical collision, and each of the three blades has its own
/// cylindrical collision object. The blades are rotated around the shell's center point at the
/// angular speed defined by param setting 1.
class ObjectPropeller final : public ObjectCollidable {
public:
    ObjectPropeller(const System::MapdataGeoObj &params);
    ~ObjectPropeller() override;

    /// @addr{0x80764EB4}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the propeller's angular velocity and rotation matrix. The rotation
    /// direction is flipped if param setting 2 is set to 1.
    void init() override {
        initAngVel();
        m_initMat.makeR(rot());
        m_initMat.setBase(3, pos());
        m_axis = m_initMat.base(2);
    }

    /// @addr{0x80765068}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        calcAngleAndRot();
    }

    /// @addr{0x80765BC0}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    void createCollision() override;
    void calcCollisionTransform() override;
    [[nodiscard]] f32 getCollisionRadius() const override;
    bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) override;

private:
    /// @addr{0x80765558}
    /// @brief Initializes the propeller's angular velocity
    void initAngVel() {
        ASSERT(m_mapObj);
        m_angVel = static_cast<f32>(static_cast<s16>(m_mapObj->setting(0)));
        if (m_mapObj->setting(1) == 1) {
            m_angVel = -m_angVel;
        }
    }

    void calcAngleAndRot();

    f32 m_angVel;             ///< Angular speed of the propeller in degrees per frame
    f32 m_angle;              ///< Accumulated rotation angle of the propeller in degrees
    EGG::Vector3f m_axis;     ///< Forward direction, which is the axis of rotation for the blades
    EGG::Matrix34f m_initMat; ///< Initial rotation/translation matrix
    EGG::Matrix34f m_curRot;  ///< Current rotation matrix of the propeller
    std::array<ObjectCollisionCylinder *, 3> m_blades; ///< The collision objects for each blade
};

} // namespace Kinoko::Field
