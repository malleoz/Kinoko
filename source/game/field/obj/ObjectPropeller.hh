#pragma once

#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Rotating Koopa shell lasers on Koopa Cape
/// @details The shell itself has cylindrical collision, and each of the three blades has its own
/// cylindrical collision object. The blades rotate around the shell's center point at the angular
/// speed defined by param setting 1.
class ObjectPropeller final : public ObjectCollidable {
public:
    /// @addr{0x80764CC8}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes @ref m_angle to zero and sets all blade collision pointers to
    /// `nullptr`.
    ObjectPropeller(const System::MapdataGeoObj &params) : ObjectCollidable(params), m_angle(0.0f) {
        m_blades.fill(nullptr);
    }

    /// @addr{0x80764E34}
    /// @brief Virtual destructor that destroys each of the propeller blades' collision object
    ~ObjectPropeller() override {
        for (auto *&blade : m_blades) {
            EGG::egg_delete(blade);
        }
    }

    /// @addr{0x80764EB4}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the propeller's angular velocity and initial transformation matrix.
    /// Stores the propeller's axis of rotation to @ref m_axis based off of @ref m_initMat.
    void init() override {
        initAngVel();
        m_initMat.makeR(rot());
        m_initMat.setBase(3, pos());
        m_axis = m_initMat.base(2);
    }

    /// @addr{0x80765068}
    /// @copybrief ObjectBase::calc()
    /// @details Dispatches to @ref calcAngleAndRot() to update the propeller's rotation angle and
    /// transformation matrix.
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

    /// @addr{0x80765A54}
    /// @copybrief ObjectCollidable::checkCollision()
    /// @param lhs The object to check collision against (usually the player)
    /// @param dist If a collision occurs, set to the distance between the two objects
    /// @return Whether or not a collision occurred
    /// @details Checks collision against each of the 3 blades and sums the resulting distance
    /// vectors
    bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) override {
        EGG::Vector3f dist0 = EGG::Vector3f::zero;
        EGG::Vector3f dist1 = EGG::Vector3f::zero;
        EGG::Vector3f dist2 = EGG::Vector3f::zero;

        bool hasCol = lhs->check(*m_blades[0], dist0);
        hasCol = hasCol || lhs->check(*m_blades[1], dist1);
        hasCol = hasCol || lhs->check(*m_blades[2], dist2);

        dist = dist0 + dist1 + dist2;

        return hasCol;
    }

private:
    /// @addr{0x80765558}
    /// @brief Initializes the propeller's angular velocity
    /// @details @ref m_angVel is set based on param setting 1, and its direction is flipped if
    /// param setting 2 is set to 1.
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
